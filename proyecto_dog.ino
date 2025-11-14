//-----BOTONES----
#define BOTON_UP 25
#define BOTON_DOWN 26
#define BOTON_LEFT 27
#define BOTON_RIGHT 35
#define BOTON_OK 13


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
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>


//------VARIABLES------
int estadoPantalla = P0;
int opcionSeleccionada = 1;  // opción actual del menú (1 a 5)
int umbralTH = 25;
int umbralHUM = 25;
int selectorTH = 0;
int umbralLDR = 0;
int umbralGAS = 0;
int gmt = 0;
unsigned long ultimoRefreshHora = 0;
const char* ssid = "MECA-IoT";
const char* password = "IoT$2026";
bool pantallaMostrada = false;
int intervaloMqtt = 30;
unsigned long ultimoTelegram = 0;
unsigned long intervaloTelegram = 5000;
TaskHandle_t TaskTelegram;



//-----OBJETOS------
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);
hd44780_I2Cexp lcd;  // auto-detect address
ESP32Time rtc;


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
  lcd.print("TEMP: ");
  lcd.print(umbralTH);
  lcd.print("C");
  if (selectorTH == 0) lcd.print(" <");  // selección TEMP


  lcd.setCursor(0, 1);
  lcd.print("HUM: ");
  lcd.print(umbralHUM);
  lcd.print("%");
  if (selectorTH == 1) lcd.print(" <");  // selección HUM


  pantallaMostrada = true;
}


void mostrarMenuLDR() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("VU LDR: ");
  lcd.print(umbralLDR);
  lcd.print("%");
  pantallaMostrada = true;
}


void mostrarMenuGAS() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("VU GAS: ");
  lcd.print(umbralGAS);
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


// -------- CONFIGURACIÓN INICIAL --------
void setup() {
  Wire.begin(21, 22);
  lcd.begin(16, 2);
  lcd.backlight();


  Serial.begin(115200);
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
  pinMode(BOTON_RIGHT, INPUT);
  pinMode(BOTON_OK, INPUT_PULLUP);


  actualizarHoraGMT();
  xTaskCreatePinnedToCore(
    TaskTelegramCode,
    "TaskTelegram",
    8000,
    NULL,
    1,
    NULL,
    0   // núcleo 0
);
}

void TaskTelegramCode(void *pvParameters) {
  while(true) {
    revisarMensajesTelegram();
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void revisarMensajesTelegram() {
  
  // Pide mensajes nuevos (no bloqueante)
  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  if (numNewMessages == 0) return;

  for (int i = 0; i < numNewMessages; i++) {
    
    // Guarda el ID del último mensaje
    bot.last_message_received = bot.messages[i].update_id;

    // Respondemos "Hola" cuando recibimos cualquier mensaje
    bot.sendMessage(CHAT_ID, "Hola", "");

    Serial.println("Respondí mensaje Telegram: Hola");
  }
}


// -------- LÓGICA PRINCIPAL --------
void loop() {
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
        estadoPantalla = P1;
        pantallaMostrada = false;
      }
      break;


    case RESTA_TH:
      if (digitalRead(BOTON_RIGHT) == HIGH) {
        if (selectorTH == 0 && umbralTH > 0) umbralTH--;
        if (selectorTH == 1 && umbralHUM > 0) umbralHUM--;
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
        estadoPantalla = P2;
        pantallaMostrada = false;
      }
      break;


    case RESTA_LDR:
      if (digitalRead(BOTON_RIGHT) == HIGH) {
        if (umbralLDR > 0) umbralLDR--;
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


      if (digitalRead(BOTON_OK) == LOW) {
        estadoPantalla = ESPERA3;
        pantallaMostrada = true;
        delay(200);
      }
      break;


    case SUMA_GAS:
      if (digitalRead(BOTON_LEFT) == HIGH) {
        if (umbralGAS < 100) umbralGAS++;
        estadoPantalla = P3;
        pantallaMostrada = false;
      }
      break;


    case RESTA_GAS:
      if (digitalRead(BOTON_RIGHT) == HIGH) {
        if (umbralGAS > 0) umbralGAS--;
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
        actualizarHoraGMT();
        estadoPantalla = P4;
        pantallaMostrada = false;
      }
      break;

    case SUMA_GMT:
      if (digitalRead(BOTON_LEFT) == HIGH) {
        if (gmt < 12) gmt++;
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
        estadoPantalla = P5;
        pantallaMostrada = false;
      }
      break;


    case SUMA_INT:  // SUMA_INTERVALO
      if (digitalRead(BOTON_LEFT) == HIGH) {
        if (intervaloMqtt < 3600) intervaloMqtt += 30;
        estadoPantalla = P5;
        pantallaMostrada = false;
      }
      break;
  }
}



  
