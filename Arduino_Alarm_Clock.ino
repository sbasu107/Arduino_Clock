#include "arduino_secrets.h"

// Date and time functions using a PCF8523 RTC connected via I2C and Wire lib
#include "RTClib.h"
#include <Arduino.h>
#include <TM1637Display.h>
#include <Servo.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Module connection pins (Digital Pins)
#define CLK 2
#define DIO 3
#define TEST_DELAY 2000

// Define AM and PM Pins
const int am_pin = A2;
const int pm_pin = A3;
const int sensor_pin = A7;

const uint8_t SEG_DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
};

TM1637Display display(CLK, DIO);
RTC_PCF8523 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void setup() {
  Serial.begin(9600);
  pinMode(am_pin, OUTPUT);
  pinMode(pm_pin, OUTPUT);
  pinMode(sensor_pin, INPUT);

#ifndef ESP8266
  while (!Serial);
#endif

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (!rtc.initialized() || rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  rtc.start();

  float drift = 43; // seconds
  float period_sec = 7 * 86400;
  float deviation_ppm = (drift / period_sec * 1000000);
  float drift_unit = 4.069;
  int offset = round(deviation_ppm / drift_unit);
  Serial.print("Offset is "); Serial.println(offset);
}

void loop() {
  DateTime now = rtc.now();

  int sensor = analogRead(sensor_pin);
  Serial.println(sensor);

  // Adjust brightness dynamically based on ambient light (sensor value 150â550)
  if (sensor < 150) {
    display.setBrightness(0x01);
  } else if (sensor >= 550) {
    display.setBrightness(0x0f);
  } else {
    int level = map(sensor, 150, 550, 1, 15);
    display.setBrightness(constrain(level, 1, 15));
  }

  int hour = now.hour();
  int minute = now.minute();

  if (hour == 0) {
    digitalWrite(am_pin, HIGH);
    digitalWrite(pm_pin, LOW);
    hour = 12;
  } else if (hour >= 1 && hour <= 11) {
    digitalWrite(am_pin, HIGH);
    digitalWrite(pm_pin, LOW);
  } else if (hour > 12 && hour <= 23) {
    digitalWrite(pm_pin, HIGH);
    digitalWrite(am_pin, LOW);
    hour -= 12;
  }

  int time = hour * 100 + minute;
  display.showNumberDecEx(time, 0b01000000, false, 4, 0);
  delay(TEST_DELAY);

  Serial.print(now.year(), DEC); Serial.print('/');
  Serial.print(now.month(), DEC); Serial.print('/');
  Serial.print(now.day(), DEC); Serial.print(" (");
  Serial.print(daysOfTheWeek[now.dayOfTheWeek()]); Serial.print(") ");
  Serial.print(now.hour(), DEC); Serial.print(':');
  Serial.print(now.minute(), DEC); Serial.print(':');
  Serial.print(now.second(), DEC); Serial.println();

  Serial.print(" since midnight 1/1/1970 = "); Serial.print(now.unixtime());
  Serial.print("s = "); Serial.print(now.unixtime() / 86400L); Serial.println("d");

  DateTime future(now + TimeSpan(7, 12, 30, 6));
  Serial.print(" now + 7d + 12h + 30m + 6s: ");
  Serial.print(future.year(), DEC); Serial.print('/');
  Serial.print(future.month(), DEC); Serial.print('/');
  Serial.print(future.day(), DEC); Serial.print(' ');
  Serial.print(future.hour(), DEC); Serial.print(':');
  Serial.print(future.minute(), DEC); Serial.print(':');
  Serial.print(future.second(), DEC); Serial.println();

  delay(1500);
}

void setRTC(uint8_t hours, uint8_t minutes, uint8_t seconds) {
  DateTime now = rtc.now();
  DateTime newTime(now.year(), now.month(), now.day(), hours, minutes, seconds);
  rtc.adjust(newTime);
}