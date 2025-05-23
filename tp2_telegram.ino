#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <Wire.h>

#define SW1 35
#define SW2 34
#define DHTPIN 23
#define DHTTYPE DHT11
#define BOTtoken "8144979266:AAF2a44-vr9p0QZr4ElCDBOh2q1-NJh-hwo"
#define CHAT_ID "7943275371"

U8G2_SH1106_128X64_NONAME_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "MECA-IoT";
const char* pass = "IoT$2025";

void setup() {
  Serial.begin(115200);
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  dht.begin();

  display.begin(); 
  display.clearBuffer();
  display.setFont(u8g2_font_ncenB08_tr);
  display.drawStr(0, 20, "Conectando WiFi...");
  display.sendBuffer();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  while (WiFi.status() != WL_CONNECTED) {
   display.clearBuffer();
  }
  bot.sendMessage(CHAT_ID, "Bot iniciado", "");
}

void loop() {
}
