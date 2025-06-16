#include "controller.h"

MotorController::MotorController(uint8_t step_pin, uint8_t dir_pin, TMC2209::SerialAddress addr, 
    float mm_per_rev, float mm_per_sec, float mm_per_sec_fast, float mm_per_sec2_fast):
step_pin(step_pin), dir_pin(dir_pin), addr(addr), mm_per_rev(mm_per_rev), 
mm_per_sec(mm_per_sec), mm_per_sec_fast(mm_per_sec_fast), mm_per_sec2_fast(mm_per_sec2_fast) {}
bool MotorController::init(){
    pinMode(step_pin, OUTPUT);
    pinMode(dir_pin, OUTPUT);
    tmc.setup(Serial0, 115200, addr);
    if(!tmc.isSetupAndCommunicating()) {
        return false;
    }
    if(!tmc.isCommunicating()) {
        return false;
    }
    tmc.enableAutomaticCurrentScaling();
    tmc.enableAutomaticGradientAdaptation();
    // tmc.disableAutomaticCurrentScaling();
    // tmc.disableAutomaticGradientAdaptation();
    tmc.setAllCurrentValues(100,80,100);
    tmc.setMicrostepsPerStep(microsteps);
    tmc.enableStealthChop();
    tmc.enableCoolStep();
    // tmc.disableStealthChop();
    // tmc.disableCoolStep();
    return true;
}

void MotorController::set_currents(int run, int hold, int hold_percent) {
    tmc.setAllCurrentValues(run, hold, hold_percent);
}

void MotorController::enable(bool en) {
    if(en){
        tmc.enable();
    } else {
        tmc.disable();
    }
}

void MotorController::set_stealth_chop(bool stealth_chop){
    if(stealth_chop) {
        tmc.enableStealthChop();
    } else {
        tmc.disableStealthChop();
    }
}

void MotorController::set_cool_step(bool cool_step){
    if(cool_step) {
        tmc.enableCoolStep();
    } else {
        tmc.disableCoolStep();
    }
}

bool MotorController::check_uart() {
    return tmc.isSetupAndCommunicating();
}

void MotorController::set_microsteps(int steps) {
    tmc.setMicrostepsPerStep(steps);
}

void MotorController::set_velocity(int steps_per_period) {
    tmc.moveAtVelocity(steps_per_period);
}

void MotorController::set_step_dir() {
    tmc.moveUsingStepDirInterface();
}

