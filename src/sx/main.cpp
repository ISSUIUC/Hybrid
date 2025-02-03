#include "E22.h"
#include "pins.h"
#include <Arduino.h>
#include <SPI.h>

SX1268 radio(SPI, E22_CS, E22_BUSY, E22_DI01, E22_RXEN, E22_RESET);

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


    radio.setup();
    radio.set_frequency(430000000);
    radio.set_modulation_params(8, LORA_BW_250, LORA_CR_4_8, false);
    radio.set_tx_power(22);
}

void loop() {
    radio.send((uint8_t*)"hello", 5);
    delay(1000);
}