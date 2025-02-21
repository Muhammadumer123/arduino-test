/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete instructions at https://RandomNerdTutorials.com/esp32-websocket-server-sensor/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/
#include "HX711.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
// #include <Adafruit_BME280.h>
// #include <Adafruit_Sensor.h>

// Replace with your network credentials
// const char* ssid = "Redmi Note 11";
// const char* password = "pakistan123";

const char* ssid = "ESP32-AP";
const char* password = "12345678";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Json Variable to Hold Sensor Readings
JSONVar readings;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

// Create a sensor object
// Adafruit_BME280 bme;

// Init BME280
// Define pins for each HX711 amplifier
const int LOADCELL_1_DT = 12;   // Data pin for load cell 1
const int LOADCELL_1_SCK = 13;  // Clock pin for load cell 1
const int LOADCELL_2_DT = 4;   // Data pin for load cell 2
const int LOADCELL_2_SCK = 5;  // Clock pin for load cell 2
const int LOADCELL_3_DT = 18;   // Data pin for load cell 3
const int LOADCELL_3_SCK = 19;  // Clock pin for load cell 3
const int LOADCELL_4_DT = 25;   // Data pin for load cell 4
const int LOADCELL_4_SCK = 33;  // Clock pin for load cell 4

// Button pin for taring all scales
const int TARE_BUTTON_PIN = 15;  // Pin for tare button

// Create HX711 instances for each load cell
HX711 scale1;
HX711 scale2;
HX711 scale3;
HX711 scale4;

// Calibration factors for each load cell
float calibration_factor1 = 8400.0;  // Change this value based on calibration
float calibration_factor2 = 8400.0;
float calibration_factor3 = 8400.0;
float calibration_factor4 = 8400.0;

// Parameters for the moving average filter
const int NUM_READINGS = 10;  // Number of readings to average
float readings1[NUM_READINGS];
float readings2[NUM_READINGS];
float readings3[NUM_READINGS];
float readings4[NUM_READINGS];
int readIndex = 0;  // Current position in the array

void initBME(){
  // if (!bme.begin(0x76)) {
  //   Serial.println("Could not find a valid BME280 sensor, check wiring!");
  //   while (1);
  // }
}

// Get Sensor Readings and return JSON object
String getSensorReadings(){
   // Read weights from each load cell and apply moving average filter
  readings1[readIndex] = scale1.get_units();
  readings2[readIndex] = scale2.get_units();
  readings3[readIndex] = scale3.get_units();
  readings4[readIndex] = scale4.get_units();

  readIndex = (readIndex + 1) % NUM_READINGS;

  // Calculate the average weight for each load cell
  float weight1 = 0, weight2 = 0, weight3 = 0, weight4 = 0;
  for (int i = 0; i < NUM_READINGS; i++) {
    weight1 += readings1[i];
    weight2 += readings2[i];
    weight3 += readings3[i];
    weight4 += readings4[i];
  }
  weight1 /= NUM_READINGS;
  weight2 /= NUM_READINGS;
  weight3 /= NUM_READINGS;
  weight4 /= NUM_READINGS;

  // Calculate total weight
  float totalWeight = weight1 + weight2 + weight3 + weight4;

  // Calculate the percentage of weight for scales 1 & 2, scales 3 & 4, and scales 1 & 3 relative to the total weight
  float sum12 = weight1 + weight2;
  float sum34 = weight3 + weight4;
  float sum13 = weight1 + weight3;
  float percentage12 = (totalWeight > 0) ? (sum12 / totalWeight) * 100 : 0;
  float percentage34 = (totalWeight > 0) ? (sum34 / totalWeight) * 100 : 0;
  float percentage13 = (totalWeight > 0) ? (sum13 / totalWeight) * 100 : 0;
  Serial.print("weight1: ");
  Serial.println(weight1);
  Serial.print("weight2: ");
  Serial.println(weight2);
  Serial.print("weight3: ");
  Serial.println(weight3);
  Serial.print("weight4: ");
  Serial.println(weight4);
  readings["firstweight"] = String(weight2);//String(bme.readTemperature());
  readings["secondweight"] = String(random(300));//String(bme.readHumidity());
  readings["thirdweight"] = String(random(300));//String(bme.readPressure()/100.0F);
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    //data[len] = 0;
    //String message = (char*)data;
    // Check if the message is "getReadings"
    //if (strcmp((char*)data, "buttonClicked") == 0) {
      //if it is, send current sensor readings
      data[len] = 0;  // Null-terminate the string
    String message = (char *)data;

    if (message == "buttonClicked") {
      Serial.println("Button clicked on the web interface!");
      // Perform your action, e.g., toggle an LED
      // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
      String sensorReadings = getSensorReadings();
      Serial.print(sensorReadings);
      notifyClients(sensorReadings);
    //}
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  Serial.begin(115200);
  //initBME();
 initWiFi();
//  WiFi.softAP(ssid, password);
  Serial.println("Access Point started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
  initLittleFS();
  initWebSocket();

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  // Start server
  server.begin();
   // Initialize each HX711
  scale1.begin(LOADCELL_1_DT, LOADCELL_1_SCK);
  scale2.begin(LOADCELL_2_DT, LOADCELL_2_SCK);
  scale3.begin(LOADCELL_3_DT, LOADCELL_3_SCK);
  scale4.begin(LOADCELL_4_DT, LOADCELL_4_SCK);
  
  // Set calibration factor
  scale1.set_scale(calibration_factor1);
  scale2.set_scale(calibration_factor2);
  scale3.set_scale(calibration_factor3);
  scale4.set_scale(calibration_factor4);
  
  // Tare each scale initially
  scale1.tare();
  scale2.tare();
  scale3.tare();
  scale4.tare();
  
  // Set up the tare button with an internal pull-up resistor
  pinMode(TARE_BUTTON_PIN, INPUT_PULLUP);

  // Initialize readings arrays with zero
  for (int i = 0; i < NUM_READINGS; i++) {
    readings1[i] = 0;
    readings2[i] = 0;
    readings3[i] = 0;
    readings4[i] = 0;
  }

  Serial.println("HX711 initialization complete.");
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    String sensorReadings = getSensorReadings();
    Serial.print(sensorReadings);
    notifyClients(sensorReadings);
    lastTime = millis();
  }
  ws.cleanupClients();
}