/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

// Import required libraries
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <Wire.h>
#include <ESP8266FtpServer.h>       // Бібліотека для роботи з SPIFFS через FTP

// Set to true to define Relay as Normally Open (NO)
#define RELAY_NO    true

// Set number of relays
#define NUM_RELAYS  5

// Assign each GPIO to a relay
int relayGPIOs[NUM_RELAYS] = {5, 4, 14, 12, 13};

FtpServer ftpSrv;                   // Оголошуємо об'єкт для роботи з модулем через FTP (для налагодження HTML)

// Replace with your network credentials
const char* ssid = "Andrew_2023";
const char* password = "graviton19630301";

const char* PARAM_INPUT_1 = "relay";  
const char* PARAM_INPUT_2 = "state";

float temperature = 28.7;
float humidity = 65.3;
float pressure = 99.8;

// Set LED GPIO
const int ledPin = 2;
// Stores LED state
String ledState;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//*****************functions***********************************
String GPIO_status(int numGPIO){
  String state;
  if (!digitalRead(numGPIO)) state = "-OFF";
  else state = "-ON";
  Serial.println(state);
  return String(state);
}

String GPIO_status_inv(int numGPIO){
  String state;
  if (digitalRead(numGPIO)) state = "-OFF";
  else state = "-ON";
  Serial.println(state);
  return String(state);
}

String getTemperature(){
  temperature = temperature + 0.1;
  Serial.println(temperature);
  return String(temperature);
}
  
String getHumidity() {
  humidity = humidity + 0.1;
  Serial.println(humidity);
  return String(humidity);
}

String getPressure() {
  pressure = pressure - 0.1;
  Serial.println(pressure);
  return String(pressure);
}

String processor(const String& var){
  Serial.print(var);
  if(var == "STATE2") return GPIO_status_inv(2);
  else if(var == "STATE5") return GPIO_status(5);
  else if(var == "STATE4") return GPIO_status(4);
  else if(var == "STATE14") return GPIO_status(14);
  else if(var == "STATE12") return GPIO_status(12);
  else if(var == "STATE13") return GPIO_status(13);
  else if (var == "TEMPERATURE") return getTemperature();
  else if (var == "HUMIDITY") return getHumidity();
  else if (var == "PRESSURE") return getPressure();
  return String();
}

String relayState(int numRelay){
  if(RELAY_NO){
    if(digitalRead(relayGPIOs[numRelay-1])){
      return "";
    }
    else {
      return "checked";
    }
  }
  else {
    if(digitalRead(relayGPIOs[numRelay-1])){
      return "checked";
    }
    else {
      return "";
    }
  }
  return "";
}
//************************************************************** 

void setup(){
  Serial.begin(115200);				// Serial port for debugging purposes
  pinMode(ledPin, OUTPUT);

  if(!SPIFFS.begin()){				// Initialize SPIFFS
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  for(int i=1; i<=NUM_RELAYS; i++){	// Set all relays to off when the program starts - if set to Normally Open (NO), the relay is off when you set the relay to HIGH
    pinMode(relayGPIOs[i-1], OUTPUT);
    if(RELAY_NO){
      digitalWrite(relayGPIOs[i-1], HIGH);
    }
    else{
      digitalWrite(relayGPIOs[i-1], LOW);
    }
  }

  WiFi.begin(ssid, password);		// Connect to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  ftpSrv.begin("relay", "relay");	// Піднімаємо FTP-сервер для зручності налагодження роботи HTML (логін: relay, пароль: relay)

  Serial.println(WiFi.localIP());	// Print ESP32 Local IP Address

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){			// Route for ROOT / web page
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  server.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request){		// Route for SETUP / web page
    request->send(SPIFFS, "/setup.html", String(), false, processor);
  });
  
  server.on("/switch", HTTP_GET, [](AsyncWebServerRequest *request){	// Route for SWITCH / web page
    request->send(SPIFFS, "/switch.html", String(), false, processor);
  });

//**************** SWITCH .html *******************************************************************************************  
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {	// Send a GET request to <ESP_IP>/update?relay=<inputMessage>&state=<inputMessage2>
    String inputMessage;
    String inputParam;
    String inputMessage2;
    String inputParam2;
    // GET input1 value on <ESP_IP>/update?relay=<inputMessage>     ("GET", "/update?relay=5&state=1")
    if (request->hasParam(PARAM_INPUT_1) & request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();		// "rel4"
      inputMessage = inputMessage.substring(3);						// "4"
	  Serial.println(inputMessage);
      inputParam = PARAM_INPUT_1;			// "relay"
      inputMessage2 = request->getParam(PARAM_INPUT_2)->value();	// "1"
	  Serial.println(inputMessage2);
      inputParam2 = PARAM_INPUT_2;			// "state"
      if(RELAY_NO){
        Serial.print("NO ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], inputMessage2.toInt());	// установка вывода в значение
      }
      else{
        Serial.print("NC ");
        digitalWrite(relayGPIOs[inputMessage.toInt()-1], !inputMessage2.toInt());
      }
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage + inputMessage2);
    request->send(200, "text/plain", "OK");
  });
  
   server.on("/status", HTTP_GET, [] (AsyncWebServerRequest *request) {	// Send a GET request to <ESP_IP>/status?rel=<inputMessage>
    String inputMessage;
    String inputParam;

    // GET input1 value on <ESP_IP>/status?relay=<inputMessage>     ("GET", "/status?relay=5")
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();		// "5"
	  Serial.println(inputMessage);
      inputParam = PARAM_INPUT_1;			// "relay"
      if(RELAY_NO){
        Serial.print("NO ");
		request->send_P(200, "text/plain", GPIO_status(relayGPIOs[inputMessage.toInt()-1]).c_str());
      }
      else{
        Serial.print("NC ");
		request->send_P(200, "text/plain", GPIO_status_inv(relayGPIOs[inputMessage.toInt()-1]).c_str());
      }
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  }); 
//******************************************************************************************************************************
  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getTemperature().c_str());
  });
  
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getHumidity().c_str());
  });
  
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getPressure().c_str());
  });
  
 // server.on("/GPIO14_status", HTTP_GET, [](AsyncWebServerRequest *request){
 //   request->send_P(200, "text/plain", GPIO_status(14).c_str());
 // });

  // Start server
  server.begin();
}
 
void loop(){
  ftpSrv.handleFTP();                    // Обробник з'єднань FTP
  
}