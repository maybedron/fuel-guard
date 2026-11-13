#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <TinyGPSPlus.h>
#include "secrets.h"
#define REED_PIN 4
#define LED_PIN  2
const unsigned long HEARTBEAT_MS = 180000;   // 6 hours in real use

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);
bool capOpen = false;
unsigned long lastHb = 0;

String locationText() {
  if (gps.location.isValid())
    return "https://www.google.com/maps?q=" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
  return "location not available yet (GPS searching)";
}

bool internetOk() {
  WiFiClient c;
  bool ok = c.connect("8.8.8.8", 53, 5000);
  c.stop();
  Serial.println(ok ? "TCP check: OK" : "TCP check: FAILED");
  return ok;
}

void sendTelegram(String msg) {
  for (int attempt = 1; attempt <= 3; attempt++) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi down, restarting");
      delay(1000);
      ESP.restart();
    }
    if (!internetOk()) {
      if (attempt == 3) {
        Serial.println("No internet, restarting with a new MAC");
        delay(1000);
        ESP.restart();
      }
      delay(2000);
      continue;
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15);
    HTTPClient http;
    http.setConnectTimeout(15000);
    http.setTimeout(15000);
    http.begin(client, String("https://api.telegram.org/bot") + BOT_TOKEN + "/sendMessage");
    http.addHeader("Content-Type", "application/json");
    String body = String("{\"chat_id\":\"") + CHAT_ID + "\",\"text\":\"" + msg + "\"}";
    int code = http.POST(body);
    Serial.printf("Telegram HTTP %d (%s) attempt %d\n", code, HTTPClient::errorToString(code).c_str(), attempt);
    http.end();
    if (code == 200) return;
    delay(2000);
  }
}
void postEvent(const char* type) {
  if (WiFi.status() != WL_CONNECTED) return;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, API_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-device-key", DEVICE_KEY);
  String body = String("{\"deviceId\":\"truck01\",\"type\":\"") + type + "\",\"gpsFix\":" + (gps.location.isValid() ? "true" : "false");
  if (gps.location.isValid())
    body += ",\"lat\":" + String(gps.location.lat(), 6) + ",\"lng\":" + String(gps.location.lng(), 6);
  body += ",\"uptimeS\":" + String(millis() / 1000) + "}";
  int code = http.POST(body);
  Serial.printf("API HTTP %d\n", code);
  http.end();
}
void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);
  pinMode(REED_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);

  // Board has no MAC burned in: set a new one on every boot (first byte must be even)
  uint8_t mac[6] = {0x24, 0x0A, 0xC4, 0, 0, 0};
  uint32_t r = esp_random();
  mac[3] = r & 0xFF;
  mac[4] = (r >> 8) & 0xFF;
  mac[5] = (r >> 16) & 0xFF;
  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, mac);
  Serial.printf("set_mac result: %d (0 = OK)\n", err);
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting WiFi");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi failed, restarting");
    delay(1000);
    ESP.restart();
  }
  Serial.println("\nConnected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  sendTelegram("FuelGuard Truck 01 is ARMED and online");
}

void loop() {
  while (gpsSerial.available()) gps.encode(gpsSerial.read());

  bool nowOpen = (digitalRead(REED_PIN) == HIGH);
  if (nowOpen != capOpen) {
    delay(50);
    nowOpen = (digitalRead(REED_PIN) == HIGH);
    if (nowOpen != capOpen) {
      capOpen = nowOpen;
      digitalWrite(LED_PIN, capOpen);
      if (capOpen) {
        Serial.println("ALERT: CAP OPENED");
        postEvent("CAP_OPENED");
        sendTelegram("ALERT: Truck 01 fuel cap OPENED! Location: " + locationText());
      } else {
        Serial.println("Cap closed");
        postEvent("CAP_CLOSED");
        sendTelegram("Truck 01 fuel cap closed");
      }
    }
  }

  if (millis() - lastHb >= HEARTBEAT_MS) {
    lastHb = millis();
    Serial.println("HEARTBEAT");
    postEvent("HEARTBEAT");
    sendTelegram("HEARTBEAT: Truck 01 online. Location: " + locationText());
  }
}