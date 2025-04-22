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
    bool check_uart();
    void step() {
        digitalWrite(step_pin, HIGH);
        digitalWrite(step_pin, LOW);
    }
    
    //another function to get current position and goal position
    // void dobigthing() {

    // }
    // //the moving part
    // void step_to_position(std::vector<int> numsteps, std::vector<bool> direcs) { //2 arrays, one for direction, one for numsteps? both have to be equal numbers
    //     for (int i = 0; i < numsteps.size(); i++) {
    //         digitalWrite(dir_pin, direcs[i] ? LOW : HIGH);
    //         current_dir = direcs[i];
    //         set_step_dir();
    //         step();
    //         delay(20);
    //     }
    // }
    

    void set_dir(bool dir) {
        digitalWrite(dir_pin, dir ? LOW : HIGH);
        current_dir = dir;
    }
private:
    TMC2209 tmc;
    TMC2209::SerialAddress addr;
    uint8_t step_pin;
    uint8_t dir_pin;
    bool current_dir = false;
};