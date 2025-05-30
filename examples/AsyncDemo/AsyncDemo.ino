/*
  -----------------------
  ElegantOTA - Async Demo Example
  -----------------------

  NOTE: Make sure you have enabled Async Mode in ElegantOTA before compiling this example!
  Guide: https://docs.elegantota.pro/async-mode/

  Skill Level: Beginner

  This example provides with a bare minimal app with ElegantOTA functionality which works
  with AsyncWebServer.

  Github: https://github.com/ayushsharma82/ElegantOTA
  WiKi: https://docs.elegantota.pro

  Works with:
  - ESP8266
  - ESP32
  - RP2040 (with WiFi) (Example: Raspberry Pi Pico W)
  - RP2350 (with WiFi) (Example: Raspberry Pi Pico 2W)

  Important note for RP2040/RP2350 users:
  - RP2040/RP2350 requires LittleFS partition for the OTA updates to work. Without LittleFS partition, OTA updates will fail.
    Make sure to select Tools > Flash Size > "2MB (Sketch 1MB, FS 1MB)" option.
  - If using bare RP2040/RP2350, it requires a WiFi chip like Pico W/Pico 2W for ElegantOTA to work.

  -------------------------------

  Upgrade to ElegantOTA Pro: https://elegantota.pro
*/

#include <ESPAsyncWebServer.h>
#include "OTAserver.h"

const char* ssid = "........";
const char* password = "........";

AsyncWebServer server(80);

unsigned long ota_timestamp = 0;

void ota_progress(size_t progress, size_t size) {
  // Log every 1 second
  if(progress == 0) Serial.printf("OTA overall size bytes: %u\n", size);
	if (millis() - ota_timestamp > 1000) {
		ota_timestamp = millis();
		Serial.printf("OTA Progress bytes: %u\n", progress);
	}
}

void setup(void) {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is ElegantOTA AsyncDemo.");
  });

  ota::server_init(server, ota_progress);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {}
