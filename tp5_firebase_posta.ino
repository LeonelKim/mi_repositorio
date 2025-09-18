
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include <U8g2lib.h>
#include <DHT.h>
#include <ESP32Time.h>
#include "time.h"

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
// ------ componentes ---------
#define SW1 34
#define SW2 35


#define DHTPIN 23
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
// ------- estados ---------
#define P1 0
#define ESPERA1 1
#define P2 2
#define ESPERA2 3
#define SUMA_C 4
#define RESTA_C 5

int estadoActual = P1;

// --------- wifi -------------
#define WIFI_SSID "MECA-IoT"
#define WIFI_PASSWORD "IoT$2025"

// ------- firebase -----------
#define WEB_API_KEY "AIzaSyAIAcNjIM3FC63rsvoouxzJsoXvbV67Kj4"
#define DATABASE_URL "https://st-proyecto-50ee5-default-rtdb.firebaseio.com"
#define USER_EMAIL "48311273@est.ort.edu.ar"
#define USER_PASSWORD "kimduyon123"


UserAuth auth(WEB_API_KEY, USER_EMAIL, USER_PASSWORD);
String uid;
// -------- paths y variables para subir -------
String databasePath;
String parentPath;

String tempPath = "/temperature";
String timePath = "/timestamp";

int timestamp;

object_t jsonData, obj1, obj2;
JsonWriter writer;
// -------------- horario ----------------
ESP32Time rtc;
const char* ntpServer = "pool.ntp.org";

// ----------- temperatura ---------------
float temperatura;
float umbral = 28.0;

// ----------- firebase ---------------
FirebaseApp app;
WiFiClientSecure sslClient;
using AsyncClient = AsyncClientClass;
AsyncClient client(sslClient);
RealtimeDatabase db;

// ------- tiempo-----------
unsigned long ultimoEnvio = 0;
unsigned long intervalo = 30000;
unsigned long ahora = 0;

// ------------- funciones -------------------------
void sincronizarHora() {
  configTime(-10800, 0, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    rtc.setTimeStruct(timeinfo);
  }
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return(0);
  }
  time(&now);
  return now;
}

void manejarRespuesta(AsyncResult &r) {
  if (r.isError()) {
    Serial.printf("Error: %s (Código: %d)\n", r.error().message().c_str(), r.error().code());
  } else if (r.available()) {
    Serial.printf("Enviado: %s = %s\n", r.uid().c_str(), r.c_str());
  }
}

void enviarDatos() {
  unsigned long ahora = millis();
  if (app.ready()) {
      ultimoEnvio = ahora;
      uid = app.getUid().c_str();
      databasePath = "/UsersData/" + uid;
      
      sincronizarHora();
    
      timestamp = getTime();
      parentPath= databasePath + "/" + String(timestamp);
      
      writer.create(obj1, tempPath, temperatura);
      writer.create(obj2, timePath, timestamp);
      writer.join(jsonData, 2, obj1, obj2);

      db.set<object_t>(client, parentPath, jsonData, manejarRespuesta, "RTDB_Send_Data");
  }
}

void mostrarPantalla1() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
 
  char stringTemp[10];
  sprintf(stringTemp, "%.1f C", temperatura);
  u8g2.drawStr(0, 15, "VA:");
  u8g2.drawStr(60, 15, stringTemp);
 
  char stringUmbral[10];
  sprintf(stringUmbral, "%.1f C", umbral);
  u8g2.drawStr(0, 35, "VU:");
  u8g2.drawStr(60, 35, stringUmbral);


  u8g2.sendBuffer();
}

void mostrarPantalla2() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);

  char stringIntervalo[20];
  sprintf(stringIntervalo, "%d segs", intervalo/1000);
  u8g2.drawStr(0, 30, "Ciclo:");
  u8g2.drawStr(60, 30, stringIntervalo);

  u8g2.sendBuffer();
}
// -------------------------------------------------


void setup() {
 
  Serial.begin(115200);
  // ----------- pines -----------------
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  u8g2.begin();
  dht.begin();
  
  // ------- conectar a wifi -----------
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  delay(500);
  sslClient.setInsecure();
  sslClient.setConnectionTimeout(1000);
  sslClient.setHandshakeTimeout(5);
 
  // --------- firebase -------------
  initializeApp(client, app, getAuth(auth), manejarRespuesta, "InicioFirebase");
  app.getApp<RealtimeDatabase>(db);
  db.url(DATABASE_URL);
// --------- sync hora -------------
  sincronizarHora();
}


void loop() {
  app.loop();

  switch (estadoActual) {
    case P1:
      temperatura = dht.readTemperature();
      mostrarPantalla1();
      ahora = millis();
      if (ahora - ultimoEnvio >= intervalo) {
        enviarDatos();
      }
      if(digitalRead(SW1) == LOW && digitalRead(SW2) == LOW){
        estadoActual = ESPERA1;
      }
      break;


    case ESPERA1:
      mostrarPantalla1();
      if(digitalRead(SW1) == HIGH && digitalRead(SW2) == HIGH){
        estadoActual = P2;
      }
      break;


    case P2:
      mostrarPantalla2();
      if (digitalRead(SW1) == LOW) {
        estadoActual = SUMA_C;
      } else if (digitalRead(SW2) == LOW) {
        estadoActual = RESTA_C;
      }
      break;


    case ESPERA2:
      if(digitalRead(SW1) == HIGH && digitalRead(SW2) == HIGH){
        estadoActual = P1;
      }
      break;
   
    case SUMA_C:
      if (digitalRead(SW2) == LOW) {
        estadoActual = ESPERA2;
      } else if (digitalRead(SW1) == HIGH) {
        intervalo = intervalo + 30000;
        estadoActual = P2;
      }
      break;


    case RESTA_C:
      if (digitalRead(SW1) == LOW) {
        estadoActual = ESPERA2;
      } else if (digitalRead(SW2) == HIGH) {
        intervalo = intervalo - 30000;
        if (intervalo < 30000) {
          intervalo = 30000;
        }
        estadoActual = P2;
      }
      break;
   }
}
