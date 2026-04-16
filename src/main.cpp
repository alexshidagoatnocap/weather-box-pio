#include "HardwareSerial.h"
#include <Adafruit_BMP085.h>
#include <Adafruit_SHT31.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <stdint.h>

Adafruit_BMP085 bmp;
Adafruit_SHT31 sht3x;

constexpr int32_t WIND_VANE_PIN{13};
constexpr int32_t WIND_SPEED_PIN{32};
constexpr int32_t ADC_RESOLUTION{4095};

constexpr uint8_t SHT3X_ADDR{0x44};

constexpr int32_t LORA_RX{16};
constexpr int32_t LORA_TX{17};
constexpr int32_t LORA_NETWORK_ID{5};
constexpr int32_t LORA_ADDRESS{1};
constexpr int32_t LORA_DESTINATION{2};

HardwareSerial LoRaSerial(2);

inline float cToF(float c) { return (c * 9.0 / 5.0) + 32.0; }

inline float windVaneVoltToDirection(int rawWindVaneVoltage) {
  return (static_cast<float>(rawWindVaneVoltage) /
          static_cast<float>(ADC_RESOLUTION)) *
         360.0;
}

void sendATCommand(const String &cmd, int waitMs = 500) {
  LoRaSerial.println(cmd);
  delay(waitMs);

  while (LoRaSerial.available()) {
    Serial.write(LoRaSerial.read());
  }
}

void setupLoRa() {
  Serial.println("Configuring LoRa module...");

  sendATCommand("AT", 300);
  sendATCommand("AT+RESET", 1000);
  sendATCommand("AT+NETWORKID=" + String(LORA_NETWORK_ID), 500);
  sendATCommand("AT+ADDRESS=" + String(LORA_ADDRESS), 500);
  sendATCommand("AT+PARAMETER?", 500);

  Serial.println("LoRa module configured.\n");
}

void sendLoRaMessage(const String &message) {
  String command = "AT+SEND=" + String(LORA_DESTINATION) + "," +
                   String(message.length()) + "," + message;

  LoRaSerial.println(command);

  Serial.println("Sending via LoRa:");
  Serial.println(message);

  delay(800);
  while (LoRaSerial.available()) {
    Serial.write(LoRaSerial.read());
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);

  LoRaSerial.begin(115200, SERIAL_8N1, LORA_RX, LORA_TX);
  delay(1000);
  Serial.println("LoRa UART started");

  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {
    }
  }

  if (!sht3x.begin(SHT3X_ADDR)) {
    Serial.println("Could not find SHT30");
    while (1) {
    }
  }

  setupLoRa();
}

static uint32_t packetSequence;

void loop() {
  auto windVaneValRaw{analogRead(WIND_VANE_PIN)};
  float windDirection{windVaneVoltToDirection(windVaneValRaw)};

  // Read BMP180/BMP085
  float bmpTempC = bmp.readTemperature();
  float bmpTempF = cToF(bmpTempC);
  float pressurePa = bmp.readPressure();
  float pressurehPa = pressurePa / 100.0;

  // Read SHT30
  float shtTempC = sht3x.readTemperature();
  float shtHumidity = sht3x.readHumidity();

  // Stop this loop cycle if SHT30 read fails
  if (isnan(shtTempC) || isnan(shtHumidity)) {
    Serial.println("Failed to read from SHT30 sensor!");
    delay(2000);
    return;
  }

  // Read wind speed
  auto sensorValue = analogRead(WIND_SPEED_PIN);

  // ESP32 ADC is typically 12-bit and 3.3V-based
  float outVoltage = sensorValue * (3.3 / 4095.0);
  float wind_speed = 6.0 * outVoltage;

  float shtTempF = cToF(shtTempC);

  // Print to Serial
  Serial.println("------ Sensor Reading ------");
  Serial.printf("SHT30 Temperature: %.1f째F (%.1f째C)\n", shtTempF, shtTempC);
  Serial.printf("SHT30 Humidity: %.1f%%\n", shtHumidity);
  Serial.printf("Pressure: %.1f hPa (%.0f Pa)\n", pressurehPa, pressurePa);
  Serial.printf("Wind Speed: %.1f m/s\n", wind_speed);
  Serial.printf("Wind Direction: %.1f째 (raw: %d)\n", windDirection,
                windVaneValRaw);
  Serial.printf("BMP Temp: %.1f째F (for reference)\n", bmpTempF);

  // Build JSON payload
  StaticJsonDocument<256> doc;
  doc["sequence"] = packetSequence;
  doc["temperature_f"] = round(shtTempF * 10) / 10.0;
  doc["temperature_c"] = round(shtTempC * 10) / 10.0;
  doc["humidity"] = round(shtHumidity * 10) / 10.0;
  doc["pressure"] = round(pressurehPa * 10) / 10.0;
  doc["wind_speed"] = round(wind_speed * 10) / 10.0;
  doc["wind_direction"] = round(windDirection);
  doc["rainfall"] = 0.0; // temporarily disabled

  String jsonString;
  serializeJson(doc, jsonString);

  // Send over LoRa
  sendLoRaMessage(jsonString);

  packetSequence++;

  Serial.println("----------------------------\n");
  delay(5000);
}
