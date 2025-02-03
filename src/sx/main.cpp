#include "E22.h"
#include "pins.h"
#include <Arduino.h>
#include <SPI.h>

void setup(){
    Serial.begin(9600);
    while(!Serial.available());
    Serial.read();

    pinMode(LSM6DS3_CS, OUTPUT);
    pinMode(KX134_CS, OUTPUT);
    pinMode(ADXL355_CS, OUTPUT);
    pinMode(LIS3MDL_CS, OUTPUT);
    pinMode(BNO086_CS, OUTPUT);
    pinMode(BNO086_RESET, OUTPUT);
    pinMode(CAN_CS, OUTPUT);
    pinMode(E22_CS, OUTPUT);
    pinMode(MS5611_CS, OUTPUT);

    digitalWrite(MS5611_CS, HIGH);
    digitalWrite(LSM6DS3_CS, HIGH);
    digitalWrite(KX134_CS, HIGH);
    digitalWrite(ADXL355_CS, HIGH);
    digitalWrite(LIS3MDL_CS, HIGH);
    digitalWrite(BNO086_CS, HIGH);
    digitalWrite(CAN_CS, HIGH);
    digitalWrite(E22_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

    E22_setup();
}

void loop() {
    transmit();
}