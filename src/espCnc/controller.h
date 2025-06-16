#pragma once

#include<cstdint>
#include<TMC2209.h>

class MotorController {
public:
    MotorController(uint8_t step_pin, uint8_t dir_pin, TMC2209::SerialAddress addr, 
        float mm_per_rev, float mm_per_sec, float mm_per_sec_fast, float mm_per_sec2_fast);
    bool init();
    void enable(bool en);
    void set_microsteps(int steps);
    void set_velocity(int steps_per_period);
    void set_step_dir();
    void set_stealth_chop(bool stealth_chop);
    void set_cool_step(bool cool_step);
    bool check_uart();
    void step() {
        digitalWrite(step_pin, HIGH);
        digitalWrite(step_pin, LOW);
    }
    void set_currents(int run, int hold, int hold_percent);
    void set_dir(bool dir) {
        digitalWrite(dir_pin, dir ? LOW : HIGH);
        current_dir = dir;
    }
    int get_microsteps() {
        return microsteps;
    }
    int get_step_per_rev() {
        return 200;
    }
    int get_mm_per_rev() {
        return mm_per_rev;
    }
    int microsteps_for_pos(float pos) {
        return pos * get_step_per_rev() * get_microsteps() / get_mm_per_rev();
    }
    int microsteps_per_sec() {
        return mm_per_sec * get_step_per_rev() * get_microsteps() / get_mm_per_rev();
    }
    int microsteps_per_sec_fast() {
        return mm_per_sec_fast * get_step_per_rev() * get_microsteps() / get_mm_per_rev();
    }
    int microsteps_per_sec2_fast() {
        return mm_per_sec2_fast * get_step_per_rev() * get_microsteps() / get_mm_per_rev();
    }
private:
    TMC2209 tmc;
    TMC2209::SerialAddress addr;
    uint8_t step_pin;
    uint8_t dir_pin;
    int microsteps = 32;
    float mm_per_rev;
    float mm_per_sec;
    float mm_per_sec_fast;
    float mm_per_sec2_fast;
    bool current_dir = false;
};