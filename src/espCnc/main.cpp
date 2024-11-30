#include<Arduino.h>
#include<Adafruit_MCP4728.h>
#include<TCA9539.h>
#include<TMC2209.h>
#include "wireless.h"
#include "esp_pins.h"
#include "gcode.h"
#include "controller.h"
#include "queue.h"
#include "status.h"


constexpr uint8_t all_pins[] = { 
// Pins::EXT0,
// Pins::EXT1,
Pins::DIAG_A,
Pins::INDEX_A,
Pins::SDA_A,
Pins::STEP_A,
Pins::DIR_A,
Pins::DIAG_B, 
Pins::INDEX_B, 
Pins::SDA_B, 
Pins::LED_0, 
Pins::LED_1, 
Pins::LED_2, 
Pins::LED_3, 
Pins::STEP_B, 
Pins::DIR_B, 
Pins::IO_INT, 
Pins::IO_RESET, 
Pins::STEP_D, 
Pins::SDA_D, 
Pins::INDEX_D, 
Pins::DIAG_D, 
Pins::LDAC, 
Pins::SDA, 
Pins::SCL, 
Pins::DIR_C, 
Pins::STEP_C, 
Pins::SDA_C, 
Pins::INDEX_C, 
Pins::DIAG_C, 
Pins::DIR_D, 
Pins::BUZZER, 
};
constexpr const char* all_pins_names[] = { 
// "Pins::EXT0",
// "Pins::EXT1",
"Pins::DIAG_A",
"Pins::INDEX_A",
"Pins::SDA_A",
"Pins::STEP_A",
"Pins::DIR_A",
"Pins::DIAG_B", 
"Pins::INDEX_B", 
"Pins::SDA_B", 
"Pins::LED_0", 
"Pins::LED_1", 
"Pins::LED_2", 
"Pins::LED_3", 
"Pins::STEP_B", 
"Pins::DIR_B", 
"Pins::IO_INT", 
"Pins::IO_RESET", 
"Pins::STEP_D", 
"Pins::SDA_D", 
"Pins::INDEX_D", 
"Pins::DIAG_D", 
"Pins::LDAC", 
"Pins::SDA", 
"Pins::SCL", 
"Pins::DIR_C", 
"Pins::STEP_C", 
"Pins::SDA_C", 
"Pins::INDEX_C", 
"Pins::DIAG_C", 
"Pins::DIR_D", 
"Pins::BUZZER", 
};
constexpr uint8_t leds[] = {
    Pins::LED_0,
    Pins::LED_1,
    Pins::LED_2,
    Pins::LED_3,
};
constexpr uint8_t tcal_pins[] = {
    Pins::MS2_B,
    Pins::MS1_B,
    Pins::SPREAD_B,
    Pins::ENN_B,
    Pins::MS2_A,
    Pins::MS1_A,
    Pins::SPREAD_A,
    Pins::ENN_A,
    Pins::MS2_D,
    Pins::MS1_D,
    Pins::SPREAD_D,
    Pins::ENN_D,
    Pins::MS2_C,
    Pins::MS1_C,
    Pins::SPREAD_C,
    Pins::ENN_C,
};
constexpr const char * tcal_pins_names[] = {
    "Pins::MS2_B",
    "Pins::MS1_B",
    "Pins::SPREAD_B",
    "Pins::ENN_B",
    "Pins::MS2_A",
    "Pins::MS1_A",
    "Pins::SPREAD_A",
    "Pins::ENN_A",
    "Pins::MS2_D",
    "Pins::MS1_D",
    "Pins::SPREAD_D",
    "Pins::ENN_D",
    "Pins::MS2_C",
    "Pins::MS1_C",
    "Pins::SPREAD_C",
    "Pins::ENN_C",
};
constexpr uint8_t TCA_ADDRESS = 0x74;
constexpr uint8_t TEMP_ADDRESS = 0x44;
Adafruit_MCP4728 mcp;
TCA9539 tcal(Pins::IO_RESET, Pins::IO_INT, TCA_ADDRESS);
MotorController controllers[4] {
    MotorController(Pins::STEP_B, Pins::DIR_B, TMC2209::SERIAL_ADDRESS_1),
    MotorController(Pins::STEP_A, Pins::DIR_A, TMC2209::SERIAL_ADDRESS_0),
    MotorController(Pins::STEP_C, Pins::DIR_C, TMC2209::SERIAL_ADDRESS_2),
    MotorController(Pins::STEP_A, Pins::DIR_D, TMC2209::SERIAL_ADDRESS_3),
};
static uint8_t cmd_buffer[1<<17]{};
WirelessServer control_server(cmd_buffer, sizeof(cmd_buffer));
Queue<Command, 512> command_queue;
Queue<Status, 128> status_queue;

struct {
    int delay_us = 2000;
    std::array<int,4> position{};
} motor_config;

void panic(int code) {
    while(true){
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

void hardware_enable_all() {
    tcal.TCA9539_set_pin_val(Pins::ENN_A, TCA9539_PIN_OUT_LOW);
    tcal.TCA9539_set_pin_val(Pins::ENN_B, TCA9539_PIN_OUT_LOW);
    tcal.TCA9539_set_pin_val(Pins::ENN_C, TCA9539_PIN_OUT_LOW);
    tcal.TCA9539_set_pin_val(Pins::ENN_D, TCA9539_PIN_OUT_LOW);
}

void hardware_disable_all() {
    tcal.TCA9539_set_pin_val(Pins::ENN_A, TCA9539_PIN_OUT_HIGH);
    tcal.TCA9539_set_pin_val(Pins::ENN_B, TCA9539_PIN_OUT_HIGH);
    tcal.TCA9539_set_pin_val(Pins::ENN_C, TCA9539_PIN_OUT_HIGH);
    tcal.TCA9539_set_pin_val(Pins::ENN_D, TCA9539_PIN_OUT_HIGH);
}

void delay_until_microseconds(uint64_t start, uint64_t end) {
    if(start > end){ //overflow
        while((uint64_t)esp_timer_get_time() > end){
            NOP();
        }
    }
    while((uint64_t)esp_timer_get_time() < end){
        NOP();
    }
}

void execute_cmd(const Command& cmd) {
    if(cmd.type == CommandType::StopAll){
        hardware_disable_all();
        
    } else if(cmd.type == CommandType::StartAll){
        hardware_enable_all();
        
    } else if(cmd.type == CommandType::Enable){
        for(int i = 0; i < 4; i++){
            controllers[i].enable(cmd.enable[i]);
        }
        
    } else if(cmd.type == CommandType::SetMicrosteps){
        for(int i = 0; i < 4; i++){
            controllers[i].set_microsteps(cmd.set_microsteps[i]);
        }
    } else if(cmd.type == CommandType::SetVelocity){
        for(int i = 0; i < 4; i++){
            if(cmd.set_velocity[i] != INT_MIN) {
                controllers[i].set_velocity(cmd.set_velocity[i]);
            }
        }
    } else if(cmd.type == CommandType::SetStepDir){
        for(int i = 0; i < 4; i++){
            if(cmd.enable[i]) {
                controllers[i].set_step_dir();
            }
        }
    } else if(cmd.type == CommandType::SetSpeed){
        motor_config.delay_us = cmd.delay_us;
    } else if(cmd.type == CommandType::MoveTo){
        int diffs[4]{};
        for(int i = 0; i < 4; i++){
            diffs[i] = std::abs(motor_config.position[i] - cmd.position[i]);
            bool dir = motor_config.position[i] < cmd.position[i];
            controllers[i].set_dir(dir);
        }
        int longest = *std::max_element(std::begin(diffs), std::end(diffs));
        int delay_us = motor_config.delay_us;
        uint64_t start = esp_timer_get_time();
        for(int step = 0; step < longest; step++) {
            for(int i = 0; i < 4; i++){
                if(diffs[i] > 0){
                    controllers[i].step();
                    diffs[i]--;
                }
            }
            delay_until_microseconds(start, start + (step + 1) * delay_us);
        }
        motor_config.position = cmd.position;
    } else if(cmd.type == CommandType::Wait){
        delayMicroseconds(cmd.delay_us);
    } else if(cmd.type == CommandType::ZeroPosition){
        motor_config.position = {0,0,0,0};
    } else if(cmd.type == CommandType::StealthChop) {
        for(int i = 0; i < 4; i++){
            controllers[i].set_stealth_chop(cmd.enable[i]);
        }
    } else if(cmd.type == CommandType::CoolStep) {
        for(int i = 0; i < 4; i++){
            controllers[i].set_cool_step(cmd.enable[i]);
        }
    } else if(cmd.type == CommandType::SetStatus) {
        control_server.set_status(Status{.instruction_number = cmd.value});
    }
}

void setup(){
    Serial.begin(115200);
    
    for(int i : leds){
        pinMode(i, OUTPUT);
    }
    if(!Wire.begin(Pins::SDA, Pins::SCL)){
        panic(1);
    }
    tcal.TCA9539_init();
    for(int i = 0; i < 16; i++){
        tcal.TCA9539_set_dir(i, TCA9539_PIN_DIR_OUTPUT);
    }
    tcal.TCA9539_set_pin_val(Pins::SPREAD_B, TCA9539_PIN_OUT_LOW);
    tcal.TCA9539_set_pin_val(Pins::ENN_B, TCA9539_PIN_OUT_HIGH);
    tcal.TCA9539_set_pin_val(Pins::SPREAD_A, TCA9539_PIN_OUT_LOW);
    tcal.TCA9539_set_pin_val(Pins::ENN_A, TCA9539_PIN_OUT_HIGH);
    tcal.TCA9539_set_pin_val(Pins::SPREAD_C, TCA9539_PIN_OUT_LOW);
    tcal.TCA9539_set_pin_val(Pins::ENN_C, TCA9539_PIN_OUT_HIGH);
    tcal.TCA9539_set_pin_val(Pins::SPREAD_D, TCA9539_PIN_OUT_LOW);
    tcal.TCA9539_set_pin_val(Pins::ENN_D, TCA9539_PIN_OUT_HIGH);

    tcal.TCA9539_set_pin_val(Pins::MS1_A, TCA9539_PIN_OUT_LOW);
    tcal.TCA9539_set_pin_val(Pins::MS2_A, TCA9539_PIN_OUT_LOW);
    tcal.TCA9539_set_pin_val(Pins::MS1_B, TCA9539_PIN_OUT_HIGH);
    tcal.TCA9539_set_pin_val(Pins::MS2_B, TCA9539_PIN_OUT_LOW);
    tcal.TCA9539_set_pin_val(Pins::MS1_C, TCA9539_PIN_OUT_LOW);
    tcal.TCA9539_set_pin_val(Pins::MS2_C, TCA9539_PIN_OUT_HIGH);
    tcal.TCA9539_set_pin_val(Pins::MS1_D, TCA9539_PIN_OUT_HIGH);
    tcal.TCA9539_set_pin_val(Pins::MS2_D, TCA9539_PIN_OUT_HIGH);


    Serial0.begin(115200);
    if(!controllers[0].init()) {
        panic(5);
    }
    if(!controllers[1].init()) {
        panic(6);
    }

    control_server.on_message([](uint8_t* ptr, size_t len){
        size_t ct = len / sizeof(Command);
        if(ct * sizeof(Command) != len) {
            Serial.println("bad size message");
            return;
        }
        for(int i = 0; i < ct; i++){
            Command cmd;
            memcpy(&cmd, ptr + i * sizeof(Command), sizeof(Command));
            if(cmd.type == CommandType::StopNow) {
                hardware_disable_all();
                Command cmd;
                //flush command queue
                while(command_queue.receive(&cmd));
            } else {
                if(!command_queue.send(cmd)) {
                    Serial.println("command queue overflow");
                }
            }
        }
    });
    control_server.setup_wifi("foobar3","25mjrn15");
    hardware_enable_all();
    digitalWrite(Pins::LED_0, HIGH);
}

int delay_us = 2000;
int pos_tracker = 0;

void loop(){
    while(true) {
        Command cmd;
        if(command_queue.receive(&cmd)){
            execute_cmd(cmd);
        }
    }
    if(Serial.available()){
        int v = Serial.read();
        if(v == 'o') {
            Serial.println("Hardware enable");
            Command cmd;
            cmd.type = CommandType::StartAll;
            execute_cmd(cmd);
        } else if(v == 'l') {
            Serial.println("Hardware disable");
            Command cmd;
            cmd.type = CommandType::StopAll;
            execute_cmd(cmd);
        } else if(v == '3') {
            delay_us *= 1.1;
            Serial.print("delay = ");
            Serial.println(delay_us);
        } else if(v == '4') {
            delay_us /= 1.1;
            Serial.print("delay = ");
            Serial.println(delay_us);
        } else if(v == '5') {
            Serial.println("ON");
            Command cmd;
            cmd.type = CommandType::Enable;
            cmd.enable = {true,true,true,true};
            execute_cmd(cmd);
        } else if(v == '6') {
            Serial.println("Velocity");
            Command cmd;
            cmd.type = CommandType::SetVelocity;
            cmd.set_velocity = {delay_us, delay_us, delay_us, delay_us};
            execute_cmd(cmd);
        } else if(v == '7') {
            Serial.println("StepDir");
            Command cmd;
            cmd.type = CommandType::SetStepDir;
            cmd.enable = {true,true,true,true};
            execute_cmd(cmd);
        } else if(v == '8') {
            Serial.println("OFF");
            Command cmd;
            cmd.type = CommandType::Enable;
            cmd.enable = {false,false,false,false};
            execute_cmd(cmd);
        } else if(v == '9') {
            pos_tracker += 1600;
            Command cmd;
            Serial.print("Pos "); Serial.println(pos_tracker);
            cmd.type = CommandType::MoveTo;
            cmd.position = {pos_tracker,pos_tracker,0,0};
            execute_cmd(cmd);
        } else if(v == '0') {
            pos_tracker -= 1600;
            Serial.print("Pos "); Serial.println(pos_tracker);
            Command cmd;
            cmd.type = CommandType::MoveTo;
            cmd.position = {pos_tracker,pos_tracker,0,0};
            execute_cmd(cmd);
        } else if(v == 's') {
            Serial.print("Speed "); Serial.println(delay_us);
            Command cmd;
            cmd.type = CommandType::SetSpeed;
            cmd.delay_us = delay_us;
            execute_cmd(cmd);
        }
    }
}

