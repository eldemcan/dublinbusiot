#include "LiquidCrystal_I2C.h"
#include "RestClient.h"
#include "ArduinoJson.h"
#include "NTPClient.h"
#include "ESP8266WiFi.h"
#include "WiFiUdp.h"
#include "TaskScheduler.h"

const char* serverAddress = "192.168.1.33";
const char* wifiSid = "skyconway_com-2263";
const char* wifiPassword = "labt5c5ghc";
const char* luasDataEndPoint = "/luas/tallaght";
const char* weatherDataEndPoint = "/weather/Dublin,IE";
const char* dublinbusDataEndPoint = "/bus/4646";
const int MAXLINENUMBER = 3; // max line number for transportation information
const int lcdWidth = 20;
const int lcdHeight = 4;

const int   NtpOffset = 3600;      // In seconds
const int   NtpInterval =  60000;    // In miliseconds
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

//Tasks
Task taskGetTransportationData(10000, TASK_FOREVER, &getTransportationData);
Task taskGetWeatherData(20000, TASK_FOREVER, &getWeatherData);
Scheduler runner;

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
 
}

void setupNtpClient() {
  timeClient.begin();
}

void setupNetwork() {
  Serial.println("Connecting...");
  client.begin(wifiSid, wifiPassword);
  Serial.println("Connected!");
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
  lcd.noBacklight();
}

void something () {
  const int oneMinute = 60000;
  const int eightHours = oneMinute * 480;
  const int hour = timeClient.getHours();
  const int day = timeClient.getDay(); //0 is sunday 1,2,3,4,5,6
  timeClient.update();

   //weekdays
  if (day>=1 && day<=5) { 
    //busy hour more requests here
    if (hour >= 7 && hour <= 10 ) { 
      taskGetWeatherData.enable();
      taskGetTransportationData.enable();
      taskGetTransportationData.setInterval(oneMinute);
      taskGetWeatherData.setInterval(oneMinute);
    }
    else {
      sleepingMode();
    }
  }
  //weekends
  else {
     sleepingMode();
  }
}

void loop() {
  // runner.execute();
  // timeClient.update();
  // Serial.println(timeClient.getFormattedTime());
  // lcd.clear();
  // getTransportationData();
  // getWeatherData();
  // something();
  // delay(10000);

  // readDataWithButton();
  getWeatherData();
  getTransportationData();
  delay(30000);
}

void readDataWithButton() {
  const int buttonState = digitalRead(buttonPin);

  if (buttonState == HIGH) {
      clearLcdLine(0);
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

   Serial.println(weatherMessage);
   clearLcdLine(weatherDataLine);
   lcd.setCursor(0, weatherDataLine);
   lcd.print(weatherMessage);
}

String getRequest(const char* endPoint) {
  String response;
  const int statusCode = client.get(endPoint, &response);
  Serial.print("Status code:");
  Serial.println(statusCode);
  return response;
}


void getTransportationData() {
  Serial.println("Getting transportation data");
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
    Serial.print("Could not parse luas data");
  }

  if (rootJsonDublinBus.success()) {
    dublinBusDataArraySize = (rootJsonDublinBus.size() > MAXLINENUMBER) ? MAXLINENUMBER : rootJsonDublinBus.size();
  }
  else {
    Serial.print("Could not parse Dublin bus data");
  }

  Serial.print("Luas array size:");
  Serial.println(luasDataArraySize);
  Serial.print("Dublin bus array size:");
  Serial.println(dublinBusDataArraySize);
  
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

  Serial.print("commonSmallIndex:");
  Serial.println(commonSmallIndex);

  for (int i = 0; i < commonSmallIndex; i++) {
    Serial.println("Priting data for common loop");
    const char* busRoute = rootJsonDublinBus[i]["route"];
    const char* busDueTime = rootJsonDublinBus[i]["duetime"];
    const char* luasDestination = rootJsonLuas[i]["destination"];
    const char* luasDueMins = rootJsonLuas[i]["dueMins"];
    lcdMessages[i] = String(busRoute)+ ' ' + String(busDueTime) + " * " + String(luasDestination) + ' ' + String(luasDueMins);
    Serial.println(lcdMessages[i]);
  }

  //print rest of the dublin bus data
  if (commonSmallIndex == luasDataArraySize) {
    Serial.print("Printing rest of the Dublin bus data");

    for (int i = commonSmallIndex; i<dublinBusDataArraySize; i++) {
      const char* busRoute = rootJsonDublinBus[i]["route"];
      const char* busDueTime = rootJsonDublinBus[i]["duetime"];
      lcdMessages[i] = String(busRoute) + ' ' + String(busDueTime);
      Serial.println(lcdMessages[i]);
    }
  }
  //print rest of the luas data
  else {
    Serial.println("Printing rest of the luas data");
    for (int i = commonSmallIndex; i<luasDataArraySize; i++ ) {
      Serial.println("In the loop");
      const char* luasDestination = rootJsonLuas[i]["destination"];
      Serial.println("Destination");
      Serial.println(luasDestination);
      const char* luasDueMins =  rootJsonLuas[i]["dueMins"];
      lcdMessages[i] = "********" + String(luasDestination) + ' ' + String(luasDueMins);
      Serial.println(lcdMessages[i]);
    }
  }
  return lcdMessages;
}

void printLcdMessages(String messages[]) {
  Serial.println("printLcdMessages function");
  Serial.println(lineNumber);

  for(int i=0; i<lineNumber; i++) {    
    clearLcdLine(i);
    lcd.setCursor(0,i);
    lcd.print(messages[i]);
    Serial.println(messages[i]);
  }
}

void getWeatherData() {
  Serial.println("Getting weather data");
  
  const String weatherData = getRequest(weatherDataEndPoint);
  
  JsonObject& rootJson = jsonBuffer.parseObject(weatherData);
  if (!rootJson.success()) {
    Serial.println("JSON parsing failed!");
  }
  else {
    const char* description = rootJson["description"];
    const char* temperature = rootJson["temperature"];
    const char* windSpeed = rootJson["windSpeed"];
    const String concatenatedString = String(description) + ' '+ String(temperature) + (char)223 + ' ' + String(windSpeed)+"kph";
    printWeatherData(concatenatedString);
  }
}