#include "LiquidCrystal_I2C.h"
#include "ArduinoJson.h"
#include "NTPClient.h"
#include "ESP8266WiFi.h"
#include "ESP8266WiFiMulti.h"
#include "ESP8266HTTPClient.h"
#include "WiFiUdp.h"
#include "TaskScheduler.h"

const bool DEBUG = true;
const char* serverAddress = "dublinpt.herokuapp.com";
const char* wifiSid = "skyconway_com-2263";
const char* wifiPassword = "labt5c5ghc";
const char* luasDataEndPoint = "/luas/hospital";
const char* weatherDataEndPoint = "/weather/Dublin,IE";
const char* dublinbusDataEndPoint = "/bus/4646";
const int MAXLINENUMBER = 3; // max line number for transportation information
const int lcdWidth = 20;
const int lcdHeight = 4;

const int oneMinute = 60000;
const int eightHours = oneMinute * 480;

const int   NtpOffset = 3600;      // In seconds
const int   NtpInterval =  oneMinute;    // In miliseconds
const char* NtpAddress =  "0.ie.pool.ntp.org";

const int buttonPin = D5;
int buttonState = 0;

const size_t weatherJsonBufferSize = JSON_OBJECT_SIZE(3) + 80;
const size_t transportationJsonBufferSize = JSON_ARRAY_SIZE(3) + 3 * JSON_OBJECT_SIZE(2) + 80;

LiquidCrystal_I2C lcd(0x27, lcdWidth, lcdHeight);

WiFiUDP ntpUDP;
ESP8266WiFiMulti WiFiMulti;
NTPClient timeClient(ntpUDP, NtpAddress, NtpOffset, NtpInterval);

int lineNumber = MAXLINENUMBER;
bool getData = false;

// Callback methods prototypes
void getWeatherData();
void getTransportationData();
void scheduleDataTasks();

//Tasks
Task taskScheduleDataTasks(oneMinute, TASK_FOREVER, &scheduleDataTasks);

Scheduler runner;

void serialPrint(String message, int val = -2) {
  if (DEBUG) {
    Serial.print(message);
    if (val != -2) {
      Serial.println(val);
    }
    else {
      Serial.println();
    }
  }
}

void setup() {
  Serial.begin(115200);
  setupGpio();
  setupNetwork();
  setupNtpClient();
  setupLcd();
  setupTaskManager();
}

void setupGpio() {
  pinMode(buttonPin, INPUT);
}

void setupTaskManager() {
  runner.init();
  runner.addTask(taskScheduleDataTasks);
  taskScheduleDataTasks.enable();
  delay(5000);
}

void setupNtpClient() {
  timeClient.begin();
}

void setupNetwork() {
  serialPrint("Connecting...");
  WiFiMulti.addAP(wifiSid, wifiPassword);
  tryNetWorkConnectionNthTime(5);
}

void tryNetWorkConnectionNthTime(int tryCount) {

  while (WiFiMulti.run() != WL_CONNECTED && tryCount != 5) {
    if ((WiFiMulti.run() == WL_CONNECTED)) {
      serialPrint("Connected!");
    }
    else {
      serialPrint("Could not connect in setup()");
    }
    delay(1000);
    tryCount++;
    serialPrint("Retrying connection", tryCount);
  }
}

void setupLcd() {
  lcd.begin(lcdWidth, lcdHeight);
  lcd.init();
  lcd.noBlink();
  lcd.backlight();
}

void sleepingMode() {
  lcd.clear();
  lcd.noBacklight();
  Serial.flush();
}

void scheduleDataTasks () {

  if (WiFiMulti.run() != WL_CONNECTED){
    tryNetWorkConnectionNthTime(2);
  }
  else {
    const int hour = timeClient.getHours();
    const int day = timeClient.getDay(); //0 is sunday 1,2,3,4,5,6

    serialPrint("hour:", hour);
    serialPrint("day:", day);
    timeClient.update();
    serialPrint("loop heap size:", ESP.getFreeHeap());

    if  ((day >= 1 && day <= 5) && (hour >= 7 && hour <= 10)) {
      lcd.backlight();
      getTransportationData();
      getWeatherData();
    }
    else {
      sleepingMode();
    }
  }
}

void loop() {
  runner.execute();
  readDataWithButton();
}

void readDataWithButton() {
  const int buttonState = digitalRead(buttonPin);

  if (buttonState == HIGH) {
    serialPrint("Button pressed");
    lcd.clear();
    lcd.backlight();
    getTransportationData();
    getWeatherData();
    delay(1000);
  }
}

void clearLcdLine(int lineNumber) {

  for (int i = 0; i < lcdWidth; ++i) {
    lcd.setCursor(i, lineNumber);
    lcd.print(" ");
  }
}

void printWeatherData(const String weatherMessage) {
  const int weatherDataLine = lcdHeight - 1;

  clearLcdLine(weatherDataLine);
  lcd.setCursor(0, weatherDataLine);
  lcd.print(weatherMessage);
}

String getRequest(const char* endPoint) {
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    serialPrint("Connected!");
    HTTPClient http;
    http.begin(serverAddress, 80, endPoint);

    const int statusCode = http.GET();
    serialPrint("Status code:", statusCode);

    if (statusCode != HTTP_CODE_OK) {
      return "0";
    }
    const String payload = http.getString();
    serialPrint(payload);
    http.end();

    return payload;
  }
  else {
    serialPrint("Could not connect network");
    return "0";
  }
  delay(1000);
}

void getTransportationData() {
  StaticJsonBuffer<transportationJsonBufferSize> jsonBufferTram;
  StaticJsonBuffer<transportationJsonBufferSize> jsonBufferBus;

  serialPrint("Getting transportation data");

  const String dublinBusData = getRequest(dublinbusDataEndPoint);
  const String luasData = getRequest(luasDataEndPoint);

  //if there is no data break from function
  if (dublinBusData == "0" && luasData == "0") {
    return;
  }

  JsonArray& rootJsonLuas = jsonBufferTram.parseArray(luasData);
  JsonArray& rootJsonDublinBus = jsonBufferBus.parseArray(dublinBusData);

  const int isLuasJsonSuccess = rootJsonLuas.success();
  const int isBusJsonSuccess = rootJsonDublinBus.success();

  int luasDataArraySize = 0, dublinBusDataArraySize = 0;

  if (!isLuasJsonSuccess && !isBusJsonSuccess) {
    return;
  }
  else {

    if (isLuasJsonSuccess) {
      luasDataArraySize = (rootJsonLuas.size() > MAXLINENUMBER) ? MAXLINENUMBER : rootJsonLuas.size();
    }
    else {
      serialPrint("luas parse failed");
      lcd.setCursor(0, 0);
      lcd.print("Luas parse failed");
    }

    if (isBusJsonSuccess) {
      dublinBusDataArraySize = (rootJsonDublinBus.size() > MAXLINENUMBER) ? MAXLINENUMBER : rootJsonDublinBus.size();
    }
    else {
      serialPrint("bus parse failed");
      lcd.setCursor(0, 1);
      lcd.print("Bus parse failed");
    }

    serialPrint("Luas array size:", luasDataArraySize);
    serialPrint("Dublin bus array size:", dublinBusDataArraySize);

    String* messages = createLcdMessages(luasDataArraySize, dublinBusDataArraySize, rootJsonLuas, rootJsonDublinBus);
    printLcdMessages(messages);
    delete[] messages;
  }
}

int calculateRequiredLineNumber(int luasDataArraySize, int dublinBusDataArraySize) {
  const int maxlineNumberOfData = luasDataArraySize >= dublinBusDataArraySize ? luasDataArraySize : dublinBusDataArraySize;
  const int requiredLineNumber =  maxlineNumberOfData >= MAXLINENUMBER ? MAXLINENUMBER : maxlineNumberOfData;
  return requiredLineNumber;
}

// TODO check all cases i.e if luas don't return data but dublin bus returns
// imagine luas = 0 dublinbus is 3
String* createLcdMessages(int luasDataArraySize, int dublinBusDataArraySize, JsonArray& rootJsonLuas, JsonArray& rootJsonDublinBus) {
  const int commonSmallIndex = luasDataArraySize <= dublinBusDataArraySize ? luasDataArraySize : dublinBusDataArraySize;
  lineNumber = calculateRequiredLineNumber(luasDataArraySize, dublinBusDataArraySize);

  String* lcdMessages = new String[lineNumber];

  serialPrint("commonSmallIndex:", commonSmallIndex);

  for (int i = 0; i < commonSmallIndex; i++) {
    serialPrint("Priting data for common loop");
    const char* busRoute = rootJsonDublinBus[i]["route"];
    const char* busDueTime = rootJsonDublinBus[i]["duetime"];
    const char* luasDestination = rootJsonLuas[i]["destination"];
    const char* luasDueMins = rootJsonLuas[i]["dueMins"];
    lcdMessages[i] = String(busRoute) + ' ' + String(busDueTime) + " * " + String(luasDestination) + ' ' + String(luasDueMins);
    serialPrint(lcdMessages[i]);
  }

  //print rest of the dublin bus data
  if (commonSmallIndex == luasDataArraySize) {
    serialPrint("Printing rest of the Dublin bus data");

    for (int i = commonSmallIndex; i < dublinBusDataArraySize; i++) {
      const char* busRoute = rootJsonDublinBus[i]["route"];
      const char* busDueTime = rootJsonDublinBus[i]["duetime"];
      lcdMessages[i] = String(busRoute) + ' ' + String(busDueTime);
      serialPrint(lcdMessages[i]);
    }

  }
  //print rest of the luas data
  else {
    serialPrint("Printing rest of the luas data");
    for (int i = commonSmallIndex; i < luasDataArraySize; i++ ) {
      const char* luasDestination = rootJsonLuas[i]["destination"];
      const char* luasDueMins =  rootJsonLuas[i]["dueMins"];
      lcdMessages[i] = "********" + String(luasDestination) + ' ' + String(luasDueMins);
      serialPrint(lcdMessages[i]);
    }
  }

  return lcdMessages;
}

void printLcdMessages(String messages[]) {

  for (int i = 0; i < lineNumber; i++) {
    clearLcdLine(i);
    lcd.setCursor(0, i);
    lcd.print(messages[i]);
    serialPrint(messages[i]);
  }
}

void getWeatherData() {
  StaticJsonBuffer<weatherJsonBufferSize>  jsonBuffer;

  serialPrint("Getting weather data");

  const String weatherData = getRequest(weatherDataEndPoint);

  if (weatherData == "0") {
    return;
  }

  JsonObject& rootJson = jsonBuffer.parseObject(weatherData);

  if (!rootJson.success()) {
    const int weatherDataLine = lcdHeight - 1;
    serialPrint("Weather data parsing failed!");
    lcd.setCursor(0, weatherDataLine);
    lcd.print("Json parsing failed");
    return;
  }
  else {
    const char* description = rootJson["description"];
    const char* temperature = rootJson["temperature"];
    const char* windSpeed = rootJson["windSpeed"];
    const String concatenatedString = String(description) + ' ' + String(temperature) + (char)223 + ' ' + String(windSpeed) + "kph";
    printWeatherData(concatenatedString);
  }
}
