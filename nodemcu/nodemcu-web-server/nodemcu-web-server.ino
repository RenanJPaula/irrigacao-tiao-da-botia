#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <Wire.h>
#include "RTClib.h"
#include "DHT.h"
#include <vector>

const String AP_NETWORK_NAME = "irrigacao-tiao";
IPAddress apIP(192, 168, 1, 1);
IPAddress apGateway(192, 168, 1, 1);
IPAddress apSubnet(255, 255, 0, 0);

const int SDA_PIN = 5; // D1
const int SCL_PIN = 4; // D2
const int BOARD_LED = 2; // D4
const int DHT_PIN = 13; // D7
const int RELAY_PIN = 15; // D8

int relayStatus = 0;
float temp = 0;
float humidity = 0;

String networkName = "VIVO2105";
String password = "a1s2d3f4g5";

ESP8266WebServer server(80);

RTC_DS3231 rtc;

#define DHTTYPE DHT22
DHT dht(DHT_PIN, DHTTYPE);

unsigned long previousMillis = 0;
const long scheduledInterval = 5000;  



const String IRRIGATION_SCHEDULE_FILE = "irrigation_schedule.txt";

/* SETUP HARDWARE */
void setupRelayPin() {
  pinMode(BOARD_LED, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BOARD_LED, HIGH);
}

void setupRTC() {
  Wire.begin(SDA_PIN, SCL_PIN);
  rtc.begin();
}

void setupDHT() {
  pinMode(DHT_PIN, INPUT);
  dht.begin();
}

/* FILE SYSTEM STORAGE */
void setupFileSystem() {
  if (!SPIFFS.begin()) {
    Serial.println("Error mounting the file system");
    return;
  }
  Serial.println("File System Ready!");
}

String getFormatedDate() {
  DateTime now = rtc.now();
  String formatedDate = "";
  formatedDate += String(now.day()) + "/" + String(now.month()) + "/" + String(now.year());
  formatedDate += " ";
  formatedDate += String(now.hour()) + ":" + String(now.minute());
  return formatedDate;
}

void setRelayOn() {
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(BOARD_LED, LOW);
  relayStatus = 1;
}

void setRelayOff() {
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BOARD_LED, HIGH);
  relayStatus = 0;
}

void readDHT() {
  temp = dht.readTemperature();
  humidity = dht.readHumidity(); 
}

/* NETWORK CONFIG */
void setupAccessPoint() {
  WiFi.mode(WIFI_AP);
  Serial.println(WiFi.softAPConfig(apIP, apGateway, apSubnet) ? "Network Configured!" : "Network Failed!");
  Serial.println(WiFi.softAP(AP_NETWORK_NAME) ? "Access Point Ready!" : "Access Point Failed!");
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("Server IP: ");
  Serial.println(WiFi.softAPIP());
}

void setupNetwork() {
  Serial.print("Connection to: ");
  Serial.println(networkName);
  Serial.print("With password: ");
  Serial.println(password);

  WiFi.begin(networkName, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Waiting to connect...");
  }

  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void autoReconnectNetwork() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Reconnecting WIFI Network");
    setupNetwork();
    delay(1000);
  }
}

/* HTTP SERVER */
void sendStatus() {
  String response = "{";
  response += "\"irrigation\" : " + String(relayStatus) + ",";
  response += "\"temp\" : " + String(temp) + ",";
  response += "\"humidity\" : " + String(humidity) + ",";
  response += "\"now\" : \"" + getFormatedDate() + "\"";
  response += "}";

  Serial.println("response:" + response);
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/json", response);
}

void handleGetStatus() {
  Serial.println("GET /status");
  sendStatus();
}

void handleChangeIrrigationStatus() {
  Serial.println("POST /irrigation");

  String irrigation = server.arg("irrigation");
  if (irrigation == "0") {
    setRelayOff();
  } else {
    setRelayOn();
  }
  sendStatus();
}

void handleScheduleIrrigation() {
  char *newSchedules = (char*) server.arg("schedule").c_str();

  char *startTime = strtok(newSchedules, ";");
  char *endTime = strtok(NULL, ";");

  Serial.println(startTime);
  Serial.println(endTime);

  int startHour = String(strtok(startTime, ":")).toInt(); 
  int startMinutes = String(strtok(NULL, ":")).toInt();

  Serial.println(startHour);
  Serial.println(startMinutes);

  int endHour = String(strtok(endTime, ":")).toInt(); 
  int endMinutes = String(strtok(NULL, ":")).toInt();

  Serial.println(endHour);
  Serial.println(endMinutes);

  DateTime now = rtc.now();
  
  int hour = now.hour();
  int minutes = now.minute();

  if (hour >= startHour && minutes >= startMinutes && hour <= endHour && minutes <= endMinutes) {
    setRelayOn();
  } else {
    setRelayOff();
  }
  
  sendStatus();
}

void setupApplicationHttpServer() {
  // API
  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/irrigation", HTTP_POST, handleChangeIrrigationStatus);
  server.on("/irrigation/schedule", HTTP_POST, handleScheduleIrrigation);
  
  server.begin();
  Serial.println("Config server listening on port 80");
}

void execOnInterval() {
  unsigned long currentMillis = millis();

  if ((currentMillis - previousMillis) >= scheduledInterval) {
    previousMillis = currentMillis;
    readDHT();
  }
}

void loadIrrigationSchedules() {
  Serial.println("Load Irrigation Schedule");
  File file = SPIFFS.open(IRRIGATION_SCHEDULE_FILE, "r");
  
  if (!file) {
    Serial.print(IRRIGATION_SCHEDULE_FILE);
    Serial.println(" not found!");
  }

  // TODO HELLLPPPP
  int i = 0;
  const char *savedSchedules[];

  while (file.available()) {
    // "10:30;10:45"
    String line = file.readStringUntil('\n');

    if (i == 0) {
      savedSchedules = String values[line.toInt()];
    }

    i++;
  }

  for(int i = 0; i < savedSchedules.size(); i++) {
    Serial.println(savedSchedules[i]);
  }

  file.close();
}

/* MAIN */
void setup() {
  Serial.begin(115200);
  Serial.println();
  setupRelayPin();
  setupRTC();
  setupDHT();
  setupFileSystem();
  // setupAccessPoint();
  setupNetwork();
  setupApplicationHttpServer();
  loadIrrigationSchedules();
}

void loop() {
  autoReconnectNetwork();  
  server.handleClient();
  execOnInterval();
}
