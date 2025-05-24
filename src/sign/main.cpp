#include <Arduino.h>
#include <SPI.h>
#include "panel_control.h"

constexpr uint8_t rgbPins[] = {13, 47, 3, 35, 8, 6};
constexpr uint8_t addrPins[] = {38, 16, 40, 18};
constexpr uint8_t clockPin = 21;
constexpr uint8_t latchPin = 9;
constexpr uint8_t oePin = 41;

constexpr int COLUMNS = 32;

struct {
    uint64_t set_pins[8][COLUMNS];
    uint64_t clear_pins[8][COLUMNS];
    uint64_t set_row_pins[8];
    uint64_t clear_row_pins[8];
} framebuff;


void set_bit(uint64_t& bitset, uint8_t pin) {
    bitset |= (1ull << pin);
}

void place_bit(uint64_t& set, uint64_t& clear, uint8_t pin, bool val) {
    if(val) {
        set_bit(set, pin);
    } else {
        set_bit(clear, pin);
    }
}

void digitalSetAll(uint64_t set, uint64_t clear) {
    GPIO.out_w1ts = (uint32_t)(set);
    GPIO.out1_w1ts.val = (uint32_t)(set>>32);
    GPIO.out_w1tc = (uint32_t)(clear);
    GPIO.out1_w1tc.val = (uint32_t)(clear>>32);
}

void setup(void)
{
    Serial.begin(9600);
    while(true) {
        pinMode(41, OUTPUT);
        digitalWrite(41, HIGH);
        delay(500);
        digitalWrite(41, LOW);
        delay(500);
    }

    test();
    for (uint8_t pin : rgbPins)
    {
        pinMode(pin, OUTPUT);
    }
    for (uint8_t pin : addrPins)
    {
        pinMode(pin, OUTPUT);
    }
    pinMode(clockPin, OUTPUT);
    pinMode(latchPin, OUTPUT);
    pinMode(oePin, OUTPUT);

    digitalWrite(rgbPins[0], HIGH);
    digitalWrite(oePin, LOW);
    digitalWrite(latchPin, LOW);


    for(int row = 0; row < 8; row++){
        uint32_t data = 0xabcdef00ul | row;
        for(int i = 0; i < COLUMNS; i++){
            uint64_t set_pins{};
            uint64_t clear_pins{};
            for(uint8_t pin : rgbPins){
                place_bit(set_pins, clear_pins, pin, (data & (1ul << i)));
            }
            framebuff.set_pins[row][i] = set_pins;
            framebuff.clear_pins[row][i] = clear_pins;
        }
    }

    for(int row = 0; row < 8; row++){
        uint64_t set_row_pins{};
        uint64_t clear_row_pins{};
        for(int i = 0; i < 3; i++) {
            place_bit(set_row_pins, clear_row_pins, addrPins[i], row & (1 << i));
        }
        place_bit(set_row_pins, clear_row_pins, oePin, true);
        // place_bit(set_row_pins, clear_row_pins, latchPin, true);
        framebuff.set_row_pins[row] = set_row_pins;
        framebuff.clear_row_pins[row] = clear_row_pins;
    }
    SPI.begin(clockPin, rgbPins[1], rgbPins[0]);
    SPISettings settings{40000000, SPI_MSBFIRST, SPI_MODE0};
    SPI.beginTransaction(settings);
}


void loop(void)
{
    // uint32_t data = micros();
    // digitalWrite(latchPin, i&0b10);
    // uint64_t start = micros();
    for(int row = 0; row < 8; row++){
        uint32_t data = 0xabcdef00ul | row;
        // for(int i = 0; i < COLUMNS; i++){
            SPI.transfer32(data);
            // uint64_t set_pins = framebuff.set_pins[row][i];
            // uint64_t clear_pins = framebuff.clear_pins[row][i];
            // clear_pins |= (1ull << clockPin);
            // digitalSetAll(set_pins, clear_pins);
            // GPIO.out_w1ts = (1 << clockPin);
        // }
        digitalSetAll(framebuff.set_row_pins[row], framebuff.clear_row_pins[row]);
        GPIO.out1_w1ts.val = (1ul <<(oePin-32));
        GPIO.out_w1ts = 1ul << latchPin;
        GPIO.out1_w1tc.val = 1ul << (oePin-32);
        GPIO.out_w1tc = 1ul << latchPin;
    }
    // uint64_t end = micros();
    // digitalWrite(oePin, HIGH);
    // delay(1);
    // Serial.println(end-start);
    // digitalWrite(oePin, LOW);
    // SPI.endTransaction();
}
