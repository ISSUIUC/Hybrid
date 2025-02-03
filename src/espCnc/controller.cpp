#include "controller.h"

MotorController::MotorController(uint8_t step_pin, uint8_t dir_pin, TMC2209::SerialAddress addr):
step_pin(step_pin), dir_pin(dir_pin), addr(addr){}
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
    tmc.setAllCurrentValues(100,100,100);
    tmc.setMicrostepsPerStep(8);
    tmc.enableStealthChop();
    tmc.enableCoolStep();
    return true;
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

uint16_t MotorController::get_stallguard() {
    return tmc.getStallGuardResult();
}

void MotorController::set_stallguard(uint8_t thresh) {
    tmc.setStallGuardThreshold(thresh);
}


