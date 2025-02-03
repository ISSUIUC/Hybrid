#pragma once

#include<cstdint>
#include<TMC2209.h>

class MotorController {
public:
    MotorController(uint8_t step_pin, uint8_t dir_pin, TMC2209::SerialAddress addr);
    bool init();
    void enable(bool en);
    void set_microsteps(int steps);
    void set_velocity(int steps_per_period);
    void set_step_dir();
    void set_stealth_chop(bool stealth_chop);
    void set_cool_step(bool cool_step);
    uint16_t get_stallguard();
    void set_stallguard(uint8_t threshold);
    bool check_uart();
    void step() {
        digitalWrite(step_pin, HIGH);
        digitalWrite(step_pin, LOW);
    }
    void set_dir(bool dir) {
        if(dir != current_dir) {
            digitalWrite(dir_pin, dir ? LOW : HIGH);
            current_dir = dir;
        }
    }
private:
    TMC2209 tmc;
    TMC2209::SerialAddress addr;
    uint8_t step_pin;
    uint8_t dir_pin;
    bool current_dir = false;
};