#include "Adafruit_VL53L0X.h"

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

// Pines de salida
#define MOTOR_PIN 3
#define BUZZER_PIN 4  

// ---- Variables para histéresis ----
bool alarmaActiva = false;
const int DISTANCIA_ON  = 1000; // enciende motor/buzzer a 1m o menos
const int DISTANCIA_OFF = 1050; // apaga recién cuando pasa 1,05m

// ---- Variables de filtro ----
static int ultimaDistancia = -1;

// ---- Función para leer distancia con promedio ----
int leerDistanciaPromedio(int muestras = 3) {
  long suma = 0;
  int validas = 0;

  for (int i = 0; i < muestras; i++) {
    VL53L0X_RangingMeasurementData_t measure;
    lox.rangingTest(&measure, false);

    if (measure.RangeStatus != 4 && measure.RangeMilliMeter > 30) {
      suma += measure.RangeMilliMeter;
      validas++;
    }
    delay(10);
  }

  if (validas > 0) {
    return suma / validas;
  } else {
    return -1;
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(MOTOR_PIN, OUTPUT);

  Serial.println("Iniciando VL53L0X...");
  if (!lox.begin()) {
    Serial.println(F("Error al iniciar VL53L0X"));
    while (1);
  }
}

void loop() {
  int distancia = leerDistanciaPromedio();

  if (distancia != -1) {
    // ---- Filtro de rango confiable ----
    if (distancia > 1000 || distancia < 30) {
      Serial.println("Fuera de rango (descartada)");
      alarmaActiva = false;
      digitalWrite(MOTOR_PIN, LOW);
      noTone(BUZZER_PIN);
      return;
    }

    // ---- Filtro anti-salto brusco ----
    if (ultimaDistancia != -1 && abs(distancia - ultimaDistancia) > 400) {
      Serial.print("Lectura descartada por salto brusco: ");
      Serial.println(distancia);
      return;
    }
    ultimaDistancia = distancia;

    // ---- Mostrar distancia ----
    Serial.print("Distancia (mm): ");
    Serial.println(distancia);

    // ---- Histéresis ----
    if (!alarmaActiva && distancia <= DISTANCIA_ON) {
      alarmaActiva = true;
    } else if (alarmaActiva && distancia >= DISTANCIA_OFF) {
      alarmaActiva = false;
    }

    // ---- Motor ----
    digitalWrite(MOTOR_PIN, alarmaActiva ? HIGH : LOW);

    // ---- Buzzer con frecuencia variable ----
    if (alarmaActiva) {
      if (distancia <= 500) {
        tone(BUZZER_PIN, 2000); // más cerca = tono agudo
      } else {
        tone(BUZZER_PIN, 1000); // entre 0,5m y 1m = tono medio
      }
    } else {
      noTone(BUZZER_PIN); // apagar buzzer
    }

  } else {
    Serial.println("Lectura inválida");
    alarmaActiva = false;
    digitalWrite(MOTOR_PIN, LOW);
    noTone(BUZZER_PIN);
  }

}
