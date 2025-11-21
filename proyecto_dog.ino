//-----BOTONES----
#define BOTON_UP 25
#define BOTON_DOWN 26
#define BOTON_LEFT 27
#define BOTON_RIGHT 32
#define BOTON_OK 13

#define MQ4_PIN 34
#define MQ9_PIN 35

//------MAQUINA DE ESTADOS------
#define P0 50
#define ESPERA1 56
#define P1 51
#define SUMA_TH 57
#define RESTA_TH 58
#define ESPERA2 59
#define P2 52
#define SUMA_LDR 60
#define RESTA_LDR 61
#define ESPERA3 62
#define P3 53
#define SUMA_GAS 63
#define RESTA_GAS 64
#define ESPERA4 67
#define P4 54
#define SUMA_GMT 65
#define RESTA_GMT 66
#define ESPERA5 68
#define P5 55
#define SUMA_INT 69
#define RESTA_INT 70


//-----TELEGRAM------
#define BOTtoken "8144979266:AAF2a44-vr9p0QZr4ElCDBOh2q1-NJh-hwo"
#define CHAT_ID "7943275371"


//------LIBRERIAS-------
#include <WiFi.h>
#include "ESP32Time.h"
#include <Adafruit_AHTX0.h>
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Preferences.h>


//------VARIABLES------
int estadoPantalla = P0;
int opcionSeleccionada = 1;  // opción actual del menú (1 a 5)
int umbralTH = 30;
int umbralHUM = 55;
int selectorTH = 0;
int umbralLDR = 50;
int umbralMQ4 = 20;
int umbralMQ9 = 20;
int selectorGAS = 0;
int gmt = 0;
int rawValue;
int lightPercent;
unsigned long ultimoRefreshHora = 0;
const char* ssid = "OSONDOSON 2.4Ghz";
const char* password = "O46334737";
bool pantallaMostrada = false;
int intervaloMqtt = 30;
unsigned long ultimoTelegram = 0;
unsigned long intervaloTelegram = 5000;
unsigned long ultimoRefreshTH = 0;
unsigned long ultimoRefreshLDR = 0;
unsigned long ultimoRefreshGAS = 0;
TaskHandle_t TaskTelegramHandle;
float temperatura;
float humedadRaw;
int humedadMapped;
bool alertaTempEnviada = false;
bool alertaHumEnviada = false;
bool alertaLDREnviada = false;
int valorMQ4;
int valorMQ9;
int MQ4Mapped;
int MQ9Mapped;
bool alertaMQ4Enviada = false;
bool alertaMQ9Enviada = false;



//-----OBJETOS------
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
hd44780_I2Cexp lcd;  // auto-detect address
ESP32Time rtc;
Adafruit_AHTX0 aht;
Preferences prefs;

// --------- MENÚ PRINCIPAL ---------
void mostrarMenu(int opcion) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("1.TH 2.LUZ 3.GAS");
  lcd.setCursor(0, 1);
  lcd.print(" 4.GMT 5.MQTT");


  switch (opcion) {
    case 1:
      lcd.setCursor(0, 0);
      lcd.print(">");
      break;
    case 2:
      lcd.setCursor(5, 0);
      lcd.print(">");
      break;
    case 3:
      lcd.setCursor(11, 0);
      lcd.print(">");
      break;
    case 4:
      lcd.setCursor(2, 1);
      lcd.print(">");
      break;
    case 5:
      lcd.setCursor(8, 1);
      lcd.print(">");
      break;
  }
}


//---GMT----
void actualizarHoraGMT() {
  configTime(gmt * 3600, 0, "pool.ntp.org");
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    rtc.setTimeStruct(timeinfo);
  }
}




// --------- SUBMENÚS ---------
void mostrarMenuTH() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEMP:");
  lcd.print(umbralTH);
  lcd.print("C");
  if (selectorTH == 0) lcd.print("<");  // selección TEMP


  lcd.setCursor(0, 1);
  lcd.print("HUM: ");
  lcd.print(umbralHUM);
  lcd.print("%");
  if (selectorTH == 1) lcd.print("<");  // selección HUM

  lcd.setCursor(10, 0);
  lcd.print("VA:");
  lcd.print(temperatura, 0);
  lcd.print("C");

  lcd.setCursor(10, 1);
  lcd.print("VA:");
  lcd.print(humedadMapped);
  lcd.print("%");

  pantallaMostrada = true;
}


void mostrarMenuLDR() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("VU LDR: ");
  lcd.print(umbralLDR);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("VA LDR: ");
  lcd.print(lightPercent);
  lcd.print("%");
  pantallaMostrada = true;
}


void mostrarMenuGAS() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MQ-4:");
  lcd.print(umbralMQ4);
  lcd.print("%");
  if (selectorGAS == 0) lcd.print("<");
  lcd.setCursor(0, 1);
  lcd.print("MQ-9:");
  lcd.print(umbralMQ9);
  lcd.print("%");
  if (selectorGAS == 1) lcd.print("<");

  lcd.setCursor(10, 0);
  lcd.print("VA:");
  lcd.print(MQ4Mapped);
  lcd.print("%");
  lcd.setCursor(10, 1);
  lcd.print("VA:");
  lcd.print(MQ9Mapped);
  lcd.print("%");
  pantallaMostrada = true;
}


void mostrarMenuGMT() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GMT: ");
  lcd.print(gmt);
  lcd.setCursor(0, 1);
  lcd.print(rtc.getTime("%H:%M:%S"));
  pantallaMostrada = true;
}


void mostrarMenuMqtt() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INT MSJ: ");
  lcd.print(intervaloMqtt);
  lcd.print("s");
  pantallaMostrada = true;
}

void guardarConfig() {
  prefs.begin("config", false); // modo escritura

  prefs.putInt("umbralTH", umbralTH);
  prefs.putInt("umbralHUM", umbralHUM);
  prefs.putInt("umbralMQ4", umbralMQ4);
  prefs.putInt("umbralMQ9", umbralMQ9);
  prefs.putInt("umbralLDR", umbralLDR);
  prefs.putInt("gmt", gmt);
  prefs.putInt("mqtt_int", intervaloMqtt);

  prefs.end();
}

// -------- CONFIGURACIÓN INICIAL --------
void setup() {
  Wire.begin(21, 22);
  Wire1.begin(18, 19);  //sda = 18 scl = 19
  lcd.begin(16, 2);
  lcd.backlight();
  if (!aht.begin(&Wire1)) {
    Serial.println("No se pudo encontrar el AHTX0. Verifica las conexiones SDA/SCL.");
    while (1) delay(10);
  }
  Serial.println("Sensor AHTX0 detectado correctamente.");

  Serial.begin(115200);
  prefs.begin("config", true);  // Modo solo lectura para cargar

  umbralTH = prefs.getInt("umbralTH", 30);
  umbralHUM = prefs.getInt("umbralHUM", 55);
  umbralMQ4 = prefs.getInt("umbralMQ4", 20);
  umbralMQ9 = prefs.getInt("umbralMQ9", 20);
  umbralLDR = prefs.getInt("umbralLDR", 50);
  gmt = prefs.getInt("gmt", 0);
  intervaloMqtt = prefs.getInt("mqtt_int", 30);

  prefs.end();
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  // Espera hasta conectarse
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Conectado a WiFi!");
  client.setInsecure();  // Necesario para usar Telegram



  pinMode(BOTON_UP, INPUT_PULLUP);
  pinMode(BOTON_DOWN, INPUT_PULLUP);
  pinMode(BOTON_LEFT, INPUT_PULLUP);
  pinMode(BOTON_RIGHT, INPUT_PULLUP);
  pinMode(BOTON_OK, INPUT_PULLUP);


  actualizarHoraGMT();
  xTaskCreatePinnedToCore(
    TaskTelegram,
    "TaskTelegram",
    10000,
    NULL,
    1,
    &TaskTelegramHandle,
    0  // núcleo 0
  );
  xTaskCreatePinnedToCore(TaskPrincipal, "Principal", 10000, NULL, 1, NULL, 1);
}

void TaskTelegram(void* pvParameters) {

  while (true) {
    if (millis() - ultimoTelegram >= intervaloTelegram) {
      ultimoTelegram = millis();

      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      if (numNewMessages > 0) {
        for (int i = 0; i < numNewMessages; i++) {

          // Guarda el ID del último mensaje
          bot.last_message_received = bot.messages[i].update_id;
          String texto = bot.messages[i].text;
          if (texto == "estadoTH") {

            // ----- Estado Temperatura -----
            String estadoTemp;
            if (temperatura > umbralTH) {
              estadoTemp = "⚠️ *Alerta*";
            } else {
              estadoTemp = "✅ Normal";
            }

            // ----- Estado Humedad -----
            String estadoHum;
            if (humedadMapped > umbralHUM) {
              estadoHum = "⚠️ *Alerta*";
            } else {
              estadoHum = "✅ Normal";
            }

            // ----- Mensaje final -----
            String msg = "📊 *VALORES ACTUALES TH Y UMBRAL *\n\n";

            msg += "🌡 *Temperatura actual:* " + String(temperatura, 1) + "°C\n";
            msg += "🔸 *Umbral Temp:* " + String(umbralTH) + "°C\n";
            msg += "📌 *Estado:* " + estadoTemp + "\n\n";

            msg += "💧 *Humedad actual:* " + String(humedadMapped) + "%\n";
            msg += "🔹 *Umbral Hum:* " + String(umbralHUM) + "%\n";
            msg += "📌 *Estado:* " + estadoHum;

            bot.sendMessage(CHAT_ID, msg, "");

            Serial.println("Envié valoresTH");
          }
          if (texto == "estadoLUZ") {

            String estadoLUZ;
            if (lightPercent > umbralLDR) {
              estadoLUZ = "⚠️ *Alerta*";
            } else {
              estadoLUZ = "✅ Normal";
            }
            String msg1 = "📊 *VALOR ACTUAL LUZ Y UMBRAL*\n\n";

            msg1 += "💡 *Luz actual:* " + String(lightPercent, 1) + "%\n";
            msg1 += "🔸 *Umbral luz:* " + String(umbralLDR) + "%\n";
            msg1 += "📌 *Estado:* " + estadoLUZ + "\n\n";

            bot.sendMessage(CHAT_ID, msg1, "");

            Serial.println("Envié valoresLUZ");
          }
          if (texto == "estadoGAS") {

            // ----- Estado Temperatura -----
            String estadoMQ4;
            if (MQ4Mapped > umbralMQ4) {
              estadoMQ4 = "⚠️ *Alerta*";
            } else {
              estadoMQ4 = "✅ Normal";
            }

            // ----- Estado Humedad -----
            String estadoMQ9;
            if (MQ9Mapped > umbralMQ9) {
              estadoMQ9 = "⚠️ *Alerta*";
            } else {
              estadoMQ9 = "✅ Normal";
            }

            // ----- Mensaje final -----
            String msg2 = "📊 *VALORES ACTUALES GAS Y UMBRAL *\n\n";

            msg2 += "🚬 *Nivel de gas actual MQ-4:* " + String(MQ4Mapped) + "%\n";
            msg2 += "💨 *Umbral gas:* " + String(umbralMQ4) + "%\n";
            msg2 += "📌 *Estado:* " + estadoMQ4 + "\n\n";

            msg2 += "🚬 *Nivel de gas actual MQ-9:* " + String(MQ9Mapped) + "%\n";
            msg2 += "💨 *Umbral gas:* " + String(umbralMQ9) + "%\n";
            msg2 += "📌 *Estado:* " + estadoMQ9;

            bot.sendMessage(CHAT_ID, msg2, "");

            Serial.println("Envié valoresGAS");
          }
        }
      }
    }

    // ---------- ALERTA TEMPERATURA ----------
    if (temperatura > umbralTH && !alertaTempEnviada) {

      String alertaTemp = "⚠️ ALERTA TEMPERATURA\n";
      alertaTemp += "Temp actual: ";
      alertaTemp += String(temperatura, 1);
      alertaTemp += "°C\nUmbral: ";
      alertaTemp += String(umbralTH);

      bot.sendMessage(CHAT_ID, alertaTemp, "");

      alertaTempEnviada = true;
    }

    if (temperatura <= umbralTH) {
      alertaTempEnviada = false;
    }


    // ---------- ALERTA HUMEDAD ----------
    if (humedadMapped > umbralHUM && !alertaHumEnviada) {

      String alertaHum = "⚠️ ALERTA HUMEDAD\n";
      alertaHum += "Humedad actual: ";
      alertaHum += String(humedadMapped);
      alertaHum += "%\nUmbral: ";
      alertaHum += String(umbralHUM);

      bot.sendMessage(CHAT_ID, alertaHum, "");

      alertaHumEnviada = true;
    }
    if (humedadMapped <= umbralHUM) {
      alertaHumEnviada = false;
    }

    if (lightPercent > umbralLDR && !alertaLDREnviada) {
      String alertaLDR = "⚠️ ALERTA LUZ\n";
      alertaLDR += "Luz actual: ";
      alertaLDR += String(lightPercent);
      alertaLDR += "%\nUmbral: ";
      alertaLDR += String(umbralLDR);

      bot.sendMessage(CHAT_ID, alertaLDR, "");

      alertaLDREnviada = true;
    }
    if (lightPercent < umbralLDR) {
      alertaLDREnviada = false;
    }
    if (MQ4Mapped > umbralMQ4 && !alertaMQ4Enviada) {
      String alertaMQ4 = "⚠️ ALERTA GAS-MQ4\n";
      alertaMQ4 += "Nivel de gas actual: ";
      alertaMQ4 += String(MQ4Mapped);
      alertaMQ4 += "%\nUmbral: ";
      alertaMQ4 += String(umbralMQ4);

      bot.sendMessage(CHAT_ID, alertaMQ4, "");

      alertaMQ4Enviada = true;
    }
    if (MQ4Mapped < umbralMQ4) {
      alertaMQ4Enviada = false;
    }

    if (MQ9Mapped > umbralMQ9 && !alertaMQ9Enviada) {
      String alertaMQ9 = "⚠️ ALERTA GAS-MQ9\n";
      alertaMQ9 += "Nivel de gas actual: ";
      alertaMQ9 += String(MQ9Mapped);
      alertaMQ9 += "%\nUmbral: ";
      alertaMQ9 += String(umbralMQ9);

      bot.sendMessage(CHAT_ID, alertaMQ9, "");

      alertaMQ9Enviada = true;
    }
    if (MQ9Mapped < umbralMQ9) {
      alertaMQ9Enviada = false;
    }


    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}


// -------- LÓGICA PRINCIPAL --------
void TaskPrincipal(void* pvParameters) {
  while (true) {
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);

    // Valores originales
    temperatura = temp.temperature;
    humedadRaw = humidity.relative_humidity;

    // Mapear la humedad a 0–100 %
    humedadMapped = map((int)humedadRaw, 0, 100, 0, 100);

    //rawValue = analogRead(LDR);
    //lightPercent = map(rawValue, 0, 4095, 0, 100);

    valorMQ4 = analogRead(MQ4_PIN);
    MQ4Mapped = map(valorMQ4, 0, 4095, 0, 100);

    valorMQ9 = analogRead(MQ9_PIN);
    MQ9Mapped = map(valorMQ9, 0, 4095, 0, 100);


    switch (estadoPantalla) {

      // ======= MENÚ PRINCIPAL =======
      case P0:
        if (!pantallaMostrada) {
          mostrarMenu(opcionSeleccionada);
          pantallaMostrada = true;
        }


        if (digitalRead(BOTON_LEFT) == LOW) {
          opcionSeleccionada--;
          if (opcionSeleccionada < 1) opcionSeleccionada = 5;
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_RIGHT) == LOW) {
          opcionSeleccionada++;
          if (opcionSeleccionada > 5) opcionSeleccionada = 1;
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_UP) == LOW) {
          if (opcionSeleccionada >= 4) opcionSeleccionada -= 3;
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_DOWN) == LOW) {
          if (opcionSeleccionada < 3) opcionSeleccionada += 3;
          else if (opcionSeleccionada == 3) opcionSeleccionada += 2;
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_OK) == LOW) {
          switch (opcionSeleccionada) {
            case 1: estadoPantalla = ESPERA1; break;
            case 2: estadoPantalla = ESPERA2; break;
            case 3: estadoPantalla = ESPERA3; break;
            case 4: estadoPantalla = ESPERA4; break;
            case 5: estadoPantalla = ESPERA5; break;
          }
          pantallaMostrada = false;
        }
        break;


      // ======= TH =======
      case ESPERA1:
        if (digitalRead(BOTON_OK) == HIGH) {
          delay(100);
          if (estadoPantalla == ESPERA1 && pantallaMostrada)
            estadoPantalla = P0;
          else
            estadoPantalla = P1;
          pantallaMostrada = false;
        }
        break;


      case P1:
        if (!pantallaMostrada) mostrarMenuTH();

        if (millis() - ultimoRefreshTH >= 2000) {  // refresca cada 0.5s
          mostrarMenuTH();
          ultimoRefreshTH = millis();
        }

        if (digitalRead(BOTON_UP) == LOW) {
          selectorTH = 0;  // TEMP
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_DOWN) == LOW) {
          selectorTH = 1;  // HUM
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_RIGHT) == LOW) {
          estadoPantalla = RESTA_TH;
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_LEFT) == LOW) {
          estadoPantalla = SUMA_TH;
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_OK) == LOW) {
          estadoPantalla = ESPERA1;
          pantallaMostrada = true;
          delay(200);
        }
        break;


      case SUMA_TH:
        if (digitalRead(BOTON_LEFT) == HIGH) {
          if (selectorTH == 0 && umbralTH < 100) umbralTH++;
          if (selectorTH == 1 && umbralHUM < 100) umbralHUM++;
           guardarConfig();
          estadoPantalla = P1;
          pantallaMostrada = false;
        }
        break;


      case RESTA_TH:
        if (digitalRead(BOTON_RIGHT) == HIGH) {
          if (selectorTH == 0 && umbralTH > 0) umbralTH--;
          if (selectorTH == 1 && umbralHUM > 0) umbralHUM--;
           guardarConfig();
          estadoPantalla = P1;
          pantallaMostrada = false;
        }
        break;


      // ======= LDR =======
      case ESPERA2:
        if (digitalRead(BOTON_OK) == HIGH) {
          delay(100);
          if (estadoPantalla == ESPERA2 && pantallaMostrada)
            estadoPantalla = P0;
          else
            estadoPantalla = P2;
          pantallaMostrada = false;
        }
        break;


      case P2:
        if (!pantallaMostrada) mostrarMenuLDR();

        // if (millis() - ultimoRefreshLDR >= 2000) {  // refresca cada 0.5s
        //    mostrarMenuLDR();
        //   ultimoRefreshLDR = millis();
        //  }

        if (digitalRead(BOTON_RIGHT) == LOW) {
          estadoPantalla = RESTA_LDR;
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_LEFT) == LOW) {
          estadoPantalla = SUMA_LDR;
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_OK) == LOW) {
          estadoPantalla = ESPERA2;
          pantallaMostrada = true;
          delay(200);
        }
        break;


      case SUMA_LDR:
        if (digitalRead(BOTON_LEFT) == HIGH) {
          if (umbralLDR < 100) umbralLDR++;
          guardarConfig();   
          estadoPantalla = P2;
          pantallaMostrada = false;
        }
        break;


      case RESTA_LDR:
        if (digitalRead(BOTON_RIGHT) == HIGH) {
          if (umbralLDR > 0) umbralLDR--;
          guardarConfig();   
          estadoPantalla = P2;
          pantallaMostrada = false;
        }
        break;


      // ======= GAS =======
      case ESPERA3:
        if (digitalRead(BOTON_OK) == HIGH) {
          delay(100);
          if (estadoPantalla == ESPERA3 && pantallaMostrada)
            estadoPantalla = P0;
          else
            estadoPantalla = P3;
          pantallaMostrada = false;
        }
        break;


      case P3:
        if (!pantallaMostrada) mostrarMenuGAS();

        if (millis() - ultimoRefreshGAS >= 2000) {  // refresca cada 0.5s
          mostrarMenuGAS();
          ultimoRefreshGAS = millis();
        }

        if (digitalRead(BOTON_RIGHT) == LOW) {
          estadoPantalla = RESTA_GAS;
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_LEFT) == LOW) {
          estadoPantalla = SUMA_GAS;
          pantallaMostrada = false;
          delay(200);
        }

        if (digitalRead(BOTON_UP) == LOW) {
          selectorGAS = 0;  // TEMP
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_DOWN) == LOW) {
          selectorGAS = 1;  // HUM
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_OK) == LOW) {
          estadoPantalla = ESPERA3;
          pantallaMostrada = true;
          delay(200);
        }
        break;


      case SUMA_GAS:
        if (digitalRead(BOTON_LEFT) == HIGH) {
          if (selectorGAS == 0 && umbralMQ4 < 100) umbralMQ4++;
          if (selectorGAS == 1 && umbralMQ9 < 100) umbralMQ9++;
           guardarConfig();
          estadoPantalla = P3;
          pantallaMostrada = false;
        }
        break;


      case RESTA_GAS:
        if (digitalRead(BOTON_RIGHT) == HIGH) {
          if (selectorGAS == 0 && umbralMQ4 > 0) umbralMQ4--;
          if (selectorGAS == 1 && umbralMQ9 > 0) umbralMQ9--;
           guardarConfig();
          estadoPantalla = P3;
          pantallaMostrada = false;
        }
        break;


      // ======= GMT =======
      case ESPERA4:
        if (digitalRead(BOTON_OK) == HIGH) {
          delay(100);
          if (estadoPantalla == ESPERA4 && pantallaMostrada)
            estadoPantalla = P0;
          else
            estadoPantalla = P4;
          pantallaMostrada = false;
        }
        break;


      case P4:
        if (!pantallaMostrada) {
          mostrarMenuGMT();
          ultimoRefreshHora = millis();
        }
        if (millis() - ultimoRefreshHora >= 1000) {
          lcd.setCursor(0, 1);
          lcd.print(rtc.getTime("%H:%M:%S"));
          lcd.print("   ");  // limpia posibles caracteres sobrantes
          ultimoRefreshHora = millis();
        }


        if (digitalRead(BOTON_RIGHT) == LOW) {
          estadoPantalla = RESTA_GMT;
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_LEFT) == LOW) {
          estadoPantalla = SUMA_GMT;
          pantallaMostrada = false;
          delay(200);
        }


        if (digitalRead(BOTON_OK) == LOW) {
          estadoPantalla = ESPERA4;
          pantallaMostrada = true;
          delay(200);
        }
        break;


      case RESTA_GMT:
        if (digitalRead(BOTON_RIGHT) == HIGH) {
          if (gmt > -12) gmt--;
          guardarConfig();
          actualizarHoraGMT();
          estadoPantalla = P4;
          pantallaMostrada = false;
        }
        break;

      case SUMA_GMT:
        if (digitalRead(BOTON_LEFT) == HIGH) {
          if (gmt < 12) gmt++;
          guardarConfig();
          actualizarHoraGMT();
          estadoPantalla = P4;
          pantallaMostrada = false;
        }
        break;



        //-------TELEGRAM, MQTT---------
      case ESPERA5:
        if (digitalRead(BOTON_OK) == HIGH) {
          delay(100);
          if (estadoPantalla == ESPERA5 && pantallaMostrada)
            estadoPantalla = P0;
          else
            estadoPantalla = P5;
          pantallaMostrada = false;
        }
        break;
      case P5:
        if (!pantallaMostrada) mostrarMenuMqtt();

        if (digitalRead(BOTON_LEFT) == LOW) {
          estadoPantalla = SUMA_INT;
          delay(200);
        }
        if (digitalRead(BOTON_RIGHT) == LOW) {
          estadoPantalla = RESTA_INT;
          delay(200);
        }
        if (digitalRead(BOTON_OK) == LOW) {
          estadoPantalla = ESPERA5;
          pantallaMostrada = true;
          delay(200);
        }
        break;

      case RESTA_INT:  // RESTA_INTERVALO
        if (digitalRead(BOTON_RIGHT) == HIGH) {
          if (intervaloMqtt > 30) intervaloMqtt -= 30;
          guardarConfig();
          estadoPantalla = P5;
          pantallaMostrada = false;
        }
        break;


      case SUMA_INT:  // SUMA_INTERVALO
        if (digitalRead(BOTON_LEFT) == HIGH) {
          if (intervaloMqtt < 3600) intervaloMqtt += 30;
          guardarConfig();
          estadoPantalla = P5;
          pantallaMostrada = false;
        }
        break;
    }
  }
}
void loop() {
}
