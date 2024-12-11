// подключение библиотеки монитора
#include <LiquidCrystal_I2C.h>
// подключение библиотеки DHT
#include "DHT.h"

// тип датчика DHT
#define DHTTYPE DHT11

// пин датчика DHT
const int pinDHT11 = 9;
// пин аналогового выхода модуля влажности почвы
const int pinSoilMoisture = A0;
// пин аналогового выхода фоторезистора
const int pinPhotoresistor = A2;

// создание экземпляра обьекта DHT
DHT dht(pinDHT11, DHTTYPE);
// Адрес I2C 0x27, 16 столбцов и 2 строки
LiquidCrystal_I2C lcd(0x27, 16, 4);

typedef struct {
  int ledPin;
  int sensorPin;
  int relayPin;
  int lowerActivationThreshold;
  String lcdMessageString;
} SensorConfiguration;

SensorConfiguration sensors[] {
  {5,9,2,28,"Temp: %sC"}, //temperature sensor
  {6,A0,3,950,"Pochva: %s%%"}, //soil humidity sensor
  {7,A2,4,800,"Svet: %s."} //light sensor
};

void setup() {
  // запуск последовательного порта
  Serial.begin(9600);
  Serial.print("setup ");

  for (int i = 0; i < sizeof(sensors)/ sizeof(sensors[0]); i++) {
    SensorConfiguration sensor = sensors[i];
    sensorEnabling(sensor.ledPin, sensor.relayPin);
  }

  dht.begin();
  // инициализация дисплея
  lcd.init();
  lcd.backlight();
  Serial.println("setup done");
}

void loop() {
  // каждые 1 сек - получение показаний датчиков
  // и вывод на дисплей
  delay(1000);

  // влажность воздуха
  float airHumidity = dht.readHumidity();
  if (isnan(airHumidity)) {
    processErrorSensorValue(pinDHT11, airHumidity, 3);
  } else {
    showSensorInfoOnLCD("Vlaga: %s%%", 3, airHumidity);
  }

  for (int i = 0; i < sizeof(sensors) / sizeof(sensors[0]); i++) {
    processSensor(sensors[i], i);
  }
}

//Обработка датчика
void processSensor(SensorConfiguration sensorConfiguration, int sensorIndex) {
  float sensorValue = readSensorValue(sensorConfiguration.sensorPin);

  if (isnan(sensorValue)) {
    processErrorSensorValue(sensorConfiguration.sensorPin, sensorValue, sensorIndex);
  } else {
    showSensorInfoOnLCD(sensorConfiguration.lcdMessageString, sensorIndex, sensorValue);
    processSensorValue(sensorValue, sensorConfiguration.lowerActivationThreshold, sensorConfiguration.ledPin, sensorConfiguration.relayPin);
  }
}

// Считать значение с датчика
float readSensorValue(int sensorPin) {
  switch (sensorPin) {
    case pinDHT11:
      return dht.readTemperature();
    case pinSoilMoisture:
      return analogRead(pinSoilMoisture);
    case pinPhotoresistor:
      return analogRead(pinPhotoresistor);
  }
}

//Вывести информацию на ЛСД для корректных значений датчиков
void showSensorInfoOnLCD(String format, int cursorRow, float sensorValue) {
  int cursorColumn = 0;
  char buffer[format.length()];

  sprintf(buffer, format.c_str(), String(sensorValue).c_str()); //костылоь. sprintf не умеет работать с дробными типами в arduino для производительности. решение с dtostrf приводит к переполнению буфера

  lcd.setCursor(cursorColumn, cursorRow);
  lcd.print(buffer);
}

//Обработать корректные значения с датчиков
void processSensorValue(int sensorValue, int lowerActivationThreshold, int sensorLedPin, int relayPin) {
    if (sensorValue > lowerActivationThreshold) {
    digitalWrite(sensorLedPin, HIGH);
    digitalWrite(relayPin, LOW);
  } else {
    digitalWrite(sensorLedPin, LOW);
    digitalWrite(relayPin, HIGH);
  }
}

//Обработать некорректные значения с датчиков
void processErrorSensorValue(int sensorPin, float sensorValue, int sensorIndex) {
    Serial.print("Датчик на пине ");
    Serial.print(sensorPin);
    Serial.print(" показывает парашу - ");
    Serial.println(sensorValue);

    showErrorOnLCD(sensorIndex);
}

//Вывести на ЛСД информацию% что датчик сломан
void showErrorOnLCD(int cursorRow) {
  int cursorColumn = 0;
  lcd.setCursor(cursorColumn, cursorRow);
  lcd.print("Sloman          "); //костыль, если рабочий текст был длиннее этой надписи, то останутся артефакты
}

//Назначить пины лампочек на выход и выключить их
//Назначить пины для реле
void sensorEnabling(int ledPin, int relayPin) {
  // пин выход лампочек датичков
  // далее лампочки отключены
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  // пин выход реле
  pinMode(relayPin, OUTPUT);
}