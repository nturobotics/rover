#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid = "{wifi name}";
const char* password = "{wifi password}";

// UDP Broadcast Config
WiFiUDP udp;
const unsigned int udpPort = 4210;
unsigned long lastBroadcastTime = 0;
const unsigned long broadcastInterval = 3000; // Broadcast every 3 seconds

void sendIPBroadcast() {
  // 255.255.255.255 targets all connected devices on the local hotspot subnet
  udp.beginPacket(IPAddress(255, 255, 255, 255), udpPort);
  udp.print("ESP32_IP:" + WiFi.localIP().toString());
  udp.endPacket();
}

void setup() {
  Serial.begin(115200);
  
  // 1. Connect to Wi-Fi automatically on boot
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // 2. Set up ArduinoOTA for Wireless Flashing
  ArduinoOTA.setHostname("esp32cam");

  ArduinoOTA.onStart([]() {
    Serial.println("Start updating firmware...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nUpdate Finished!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
  });

  ArduinoOTA.begin();

  // 3. Send an immediate initial UDP broadcast
  sendIPBroadcast();
}

void loop() {
  // Keep OTA listener active in the background
  ArduinoOTA.handle();

  // Periodically broadcast IP address outward over UDP
  if (millis() - lastBroadcastTime >= broadcastInterval) {
    lastBroadcastTime = millis();
    sendIPBroadcast();
  }

  // Your main code goes here (e.g., camera operations)
}
