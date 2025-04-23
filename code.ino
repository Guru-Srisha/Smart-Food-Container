#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "";
char pass[] = "";

#include <Arduino.h>
#include <DHT.h>
#include "HX711.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#define SERVO_PIN 23
Servo servo;

int servoAngle = 0;

#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ135PIN 34
#define LOADCELL_DOUT_PIN 26
#define LOADCELL_SCK_PIN 25
#define SOIL_MOISTURE_PIN 35
#define ONE_WIRE_BUS 27

DHT dht(DHTPIN, DHTTYPE);
HX711 scale;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2);

float targetTemp = 25.0;

BLYNK_WRITE(V9) {
  targetTemp = param.asFloat();
}
BLYNK_WRITE(V10) {
  int value = param.asInt(); // 1 = Open, 0 = Close
  if (value == 1) {
    servoAngle = 60;
  } else {
    servoAngle = 0;
  }
  servo.write(servoAngle);
  Serial.print("Servo angle set to: ");
  Serial.println(servoAngle);
}

// Color Sensor Pin Definitions
const int S0 = 33;
const int S1 = 32;
const int S2 = 14;
const int S3 = 12;
const int sensorOut = 13;

void setup() {
  pinMode(15,OUTPUT);
  pinMode(2,OUTPUT);
  Serial.begin(115200);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  dht.begin();
  ds18b20.begin();
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  delay(1000);
servo.attach(SERVO_PIN);
servo.write(0);

  if (!scale.is_ready()) {
    Serial.println("HX711 not found.");
  } else {
    Serial.println("HX711 ready.");
    scale.set_scale();
    scale.tare();
  }

  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Container ready");

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);
  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW);
}

int readColorFrequency(int s2_val, int s3_val) {
  digitalWrite(S2, s2_val);
  digitalWrite(S3, s3_val);
  delay(100);
  return pulseIn(sensorOut, LOW);
}

void loop() {
  Blynk.run();

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) {
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print("%  Temperature (DHT11): ");
    Serial.print(t);
    Serial.println("°C");
    Blynk.virtualWrite(V0, h);
    Blynk.virtualWrite(V5, t);
  }

  int mq135Value = analogRead(MQ135PIN);
  int mq135Percent = map(mq135Value, 0, 4095, 0, 100);
  mq135Percent = constrain(mq135Percent, 0, 100);
  Serial.print("NH3 Value (%): ");
  Serial.print(mq135Percent);
  Serial.println("%");
  Blynk.virtualWrite(V1, mq135Percent);

  if (scale.is_ready()) {
    float weight = scale.get_units(5);
    
    long mappedWeight = map((long)weight, 60000, 10000, 1000, 0);
    Serial.print("Weight: ");
    Serial.print(mappedWeight);
    Serial.println(" Kg");
    Blynk.virtualWrite(V2, mappedWeight);
  } else {
    Serial.println("HX711 not ready.");
  }

  int soilValue = analogRead(SOIL_MOISTURE_PIN);
  int soilPercent = map(soilValue, 3000, 1000, 0, 100);
  soilPercent = constrain(soilPercent, 0, 100);
  Serial.print("Moisture (%): ");
  Serial.print(soilPercent);
  Serial.println("%");
  Blynk.virtualWrite(V3, soilPercent);

  ds18b20.requestTemperatures();
  float tempDS = ds18b20.getTempCByIndex(0);
  if (tempDS != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature (DS18B20): ");
    Serial.print(tempDS);
    Serial.println("°C");
    Blynk.virtualWrite(V4, tempDS);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(tempDS);

  } else {
    Serial.println("Failed to read from DS18B20!");
  }

  int redFrequency = readColorFrequency(LOW, LOW);
  int greenFrequency = readColorFrequency(HIGH, HIGH);
  int blueFrequency = readColorFrequency(LOW, HIGH);

  int redPercent = map(redFrequency, 700, 100, 0, 100);
  int greenPercent = map(greenFrequency, 700, 100, 0, 100);
  int bluePercent = map(blueFrequency, 700, 100, 0, 100);

  redPercent = constrain(redPercent, 0, 100);
  greenPercent = constrain(greenPercent, 0, 100);
  bluePercent = constrain(bluePercent, 0, 100);

  Serial.print("Color Sensor → Red (%): ");
  Serial.print(redPercent);
  Serial.print("% | Green (%): ");
  Serial.print(greenPercent);
  Serial.print("% | Blue (%): ");
  Serial.print(bluePercent);
  Serial.println("%");

  Blynk.virtualWrite(V6, redPercent);
  Blynk.virtualWrite(V7, greenPercent);
  Blynk.virtualWrite(V8, bluePercent);

  Serial.println("-----------------------");

if (targetTemp >= -10 && targetTemp <= 27) {
    digitalWrite(15, HIGH);
    digitalWrite(2, LOW);
  }
  else if (targetTemp >= 28 && targetTemp <= 50) {
    digitalWrite(2, HIGH);
    digitalWrite(15, LOW);
  }

  delay(2000);
}
