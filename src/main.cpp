#include "HardwareSerial.h"
#include <Adafruit_BMP085.h>
#include <Adafruit_SHT31.h>
#include <Arduino.h>

Adafruit_BMP085 bmp;
Adafruit_SHT31 sht3x;

void setup() {
  Serial.begin(9600);
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {
    }
  }

  if (!sht3x.begin(0x44)) {
    Serial.println("Could not find SHT30");
    while (1) {
    }
  }
}

void loop() {
  Serial.print("BMP085 Temperature = ");
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");

  Serial.print("BMP085 Pressure = ");
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");

  // Calculate altitude assuming 'standard' barometric
  // pressure of 1013.25 millibar = 101325 Pascal
  Serial.print("BMP085 Altitude = ");
  Serial.print(bmp.readAltitude());
  Serial.println(" meters");

  Serial.print("BMP085 Pressure at sealevel (calculated) = ");
  Serial.print(bmp.readSealevelPressure());
  Serial.println(" Pa");

  // you can get a more precise measurement of altitude
  // if you know the current sea level pressure which will
  // vary with weather and such. If it is 1015 millibars
  // that is equal to 101500 Pascals.
  Serial.print("BMP085 Real altitude = ");
  Serial.print(bmp.readAltitude(101500));
  Serial.println(" meters");

  Serial.print("SHT30 Temperature = ");
  Serial.print(sht3x.readTemperature());
  Serial.println(" *C");

  Serial.print("SHT30 Humidity = ");
  Serial.print(sht3x.readHumidity());
  Serial.println(" %");

  Serial.println();
  delay(500);
}
