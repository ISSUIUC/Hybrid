#pragma once

#include<Arduino.h>
#include"esp_pins.h"

inline void panic(int code) {
    Serial.print("Panic ");
    Serial.println(code);
    static constexpr uint8_t leds[] = {
        Pins::LED_0,
        Pins::LED_1,
        Pins::LED_2,
        Pins::LED_3,
    };
    
    while(true){
        Serial.print("Panic ");
        Serial.println(code);
        for(int i = 0; i < sizeof(leds); i++){
            if((1<<i)&code){
                digitalWrite(leds[i], HIGH);
            }
        }
        delay(100);
        for(int i : leds){
            digitalWrite(i, LOW);
        }
        delay(100);
    }
}
