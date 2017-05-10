#include "LiquidCrystal_I2C.h"
#include "RestClient.h"
#include "ArduinoJson.h"
#include "NTPClient.h"
#include "ESP8266WiFi.h"
#include "WiFiUdp.h"
#include "TaskScheduler.h"

const bool DEBUG = true;
const char* serverAddress = "192.168.1.33";
const char* wifiSid = "skyconway_com-2263";
const char* wifiPassword = "labt5c5ghc";
const char* luasDataEndPoint = "/luas/tallaght";
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

LiquidCrystal_I2C lcd(0x27, lcdWidth, lcdHeight);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NtpAddress, NtpOffset, NtpInterval);

RestClient client = RestClient(serverAddress, 3000);
DynamicJsonBuffer jsonBuffer;
int lineNumber = MAXLINENUMBER;

// Callback methods prototypes
void getWeatherData();
void getTransportationData();
void scheduleDataTasks();

//Tasks
Task taskGetTransportationData(oneMinute, TASK_FOREVER, &getTransportationData);
Task taskGetWeatherData(eightHours, TASK_FOREVER, &getWeatherData);
Task taskScheduleDataTasks(10000, TASK_FOREVER, &scheduleDataTasks);

Scheduler runner;

void serialPrint(String message, int val = -2) {
  if (DEBUG) {
    Serial.print(message);
      if (val!= -2){
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
  runner.addTask(taskGetWeatherData);
  runner.addTask(taskGetTransportationData);
  runner.addTask(taskScheduleDataTasks);
  taskScheduleDataTasks.enable();
  delay(5000);
}

void setupNtpClient() {
  timeClient.begin();
}

void setupNetwork() {
  serialPrint("Connecting...");
  client.begin(wifiSid, wifiPassword);
  serialPrint("Connected!");
}

void setupLcd() {
  lcd.begin(lcdWidth,lcdHeight);
  lcd.init();
  lcd.noBlink();
  lcd.backlight();
}

void sleepingMode() {
  taskGetWeatherData.disable();
  taskGetTransportationData.disable();
  lcd.clear();
  lcd.noBacklight();
}

void scheduleDataTasks () {
  const int hour = timeClient.getHours();
  const int day = timeClient.getDay(); //0 is sunday 1,2,3,4,5,6

  serialPrint("hour:", hour);
  serialPrint("day:", day);

  timeClient.update();

   //weekdays
  if (day>=1 && day<=5) {
    //busy hour more requests here
    if (hour >= 7 && hour <= 10 ) {
      serialPrint("Enabling tasks for morning");
      taskGetWeatherData.enable();
      taskGetTransportationData.enable();
    }
    else {
      sleepingMode();
    }
  }
  //weekends
  else {
    serialPrint("Weekend work on demand");
    sleepingMode();
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
      delay(10000);
  }
}

void clearLcdLine(int lineNumber) {

  for (int i = 0; i < lcdWidth; ++i) {
    lcd.setCursor(i,lineNumber);
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
  String response;
  const int statusCode = client.get(endPoint, &response);
  serialPrint("Status code:", statusCode);
  
  return response;
}

void getTransportationData() {
  serialPrint("Getting transportation data");

  const String dublinBusData = getRequest(dublinbusDataEndPoint);
  const String luasData = getRequest(luasDataEndPoint);

  JsonArray& rootJsonLuas = jsonBuffer.parseArray(luasData);
  JsonArray& rootJsonDublinBus = jsonBuffer.parseArray(dublinBusData);
  int luasDataArraySize=0;
  int dublinBusDataArraySize=0;

  if (rootJsonLuas.success()) {
    luasDataArraySize = (rootJsonLuas.size() > MAXLINENUMBER) ? MAXLINENUMBER: rootJsonLuas.size();
  }
  else {
    serialPrint("Could not parse luas data");
  }

  if (rootJsonDublinBus.success()) {
    dublinBusDataArraySize = (rootJsonDublinBus.size() > MAXLINENUMBER) ? MAXLINENUMBER : rootJsonDublinBus.size();
  }
  else {
    serialPrint("Could not parse Dublin bus data");
  }

  serialPrint("Luas array size:", luasDataArraySize);
  serialPrint("Dublin bus array size:", dublinBusDataArraySize);
  
  String* messages = createLcdMessages(luasDataArraySize, dublinBusDataArraySize, rootJsonLuas, rootJsonDublinBus);
  printLcdMessages(messages);
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
    lcdMessages[i] = String(busRoute)+ ' ' + String(busDueTime) + " * " + String(luasDestination) + ' ' + String(luasDueMins);
    serialPrint(lcdMessages[i]);
  }

  //print rest of the dublin bus data
  if (commonSmallIndex == luasDataArraySize) {
    serialPrint("Printing rest of the Dublin bus data");

    for (int i = commonSmallIndex; i<dublinBusDataArraySize; i++) {
      const char* busRoute = rootJsonDublinBus[i]["route"];
      const char* busDueTime = rootJsonDublinBus[i]["duetime"];
      lcdMessages[i] = String(busRoute) + ' ' + String(busDueTime);
      serialPrint(lcdMessages[i]);
    }
  }
  //print rest of the luas data
  else {
    serialPrint("Printing rest of the luas data");
    for (int i = commonSmallIndex; i<luasDataArraySize; i++ ) {
      const char* luasDestination = rootJsonLuas[i]["destination"];
      const char* luasDueMins =  rootJsonLuas[i]["dueMins"];
      lcdMessages[i] = "********" + String(luasDestination) + ' ' + String(luasDueMins);
      serialPrint(lcdMessages[i]);
    }
  }
  return lcdMessages;
}

void printLcdMessages(String messages[]) {

  for(int i=0; i<lineNumber; i++) {
    clearLcdLine(i);
    lcd.setCursor(0,i);
    lcd.print(messages[i]);
    serialPrint(messages[i]);
  }
}

void getWeatherData() {
  serialPrint("Getting weather data");

  const String weatherData = getRequest(weatherDataEndPoint);

  JsonObject& rootJson = jsonBuffer.parseObject(weatherData);
  if (!rootJson.success()) {
    serialPrint("JSON parsing failed!");
  }
  else {
    const char* description = rootJson["description"];
    const char* temperature = rootJson["temperature"];
    const char* windSpeed = rootJson["windSpeed"];
    const String concatenatedString = String(description) + ' '+ String(temperature) + (char)223 + ' ' + String(windSpeed)+"kph";
    printWeatherData(concatenatedString);
  }
}