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

constexpr uint8_t leds[] = {
    Pins::LED_0,
    Pins::LED_1,
    Pins::LED_2,
    Pins::LED_3,
};

constexpr uint8_t TCA_ADDRESS = 0x74;
constexpr uint8_t TEMP_ADDRESS = 0x44;
TCA9539 tcal(Pins::IO_RESET, Pins::IO_INT, TCA_ADDRESS);
MotorController controllers[4] {
    MotorController(Pins::STEP_A, Pins::DIR_A, TMC2209::SERIAL_ADDRESS_0),
    MotorController(Pins::STEP_C, Pins::DIR_C, TMC2209::SERIAL_ADDRESS_2),
    MotorController(Pins::STEP_B, Pins::DIR_B, TMC2209::SERIAL_ADDRESS_1),
    MotorController(Pins::STEP_D, Pins::DIR_D, TMC2209::SERIAL_ADDRESS_3),
};
static uint8_t cmd_buffer[1<<17]{};
static constexpr size_t TIMING_CHUNK_SIZE = (1<<12);
static constexpr size_t TIMING_BUFFER_COUNT = 16;
static TimedCommand timing_buffers[TIMING_BUFFER_COUNT][TIMING_CHUNK_SIZE]{};
WirelessServer control_server(cmd_buffer, sizeof(cmd_buffer));
Queue<Command, 512> command_queue;
Queue<TimedCommand*, TIMING_BUFFER_COUNT> free_timing_buffers;

struct {
    int delay_us = 2000;
    int64_t now_time{};
    std::array<int,4> position{};
} motor_config;

void panic(int code) {
    Serial.println("Panic");
    Serial.println(code);
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

void delay_until_microseconds(uint64_t end) {
    while((uint64_t)esp_timer_get_time() < end){
        NOP();
    }
}

int64_t execute_timing(const TimedCommand* cmds, size_t len, int64_t start_time) {
    int64_t now = start_time;
    for(int i = 0; i < len; i++){
        TimedCommand cmd = cmds[i];
        now += cmd.delay();
        delay_until_microseconds(now);
        for(int channel = 0; channel < 4; channel++){
            if(cmd.get_channel(channel)) {
                controllers[channel].set_dir(cmd.get_direction(channel));
                controllers[channel].step();
                motor_config.position[channel] += cmd.get_direction(channel) ? 1 : -1;
            }
        }
    }
    return now;
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
            delay_until_microseconds(start + (step + 1) * delay_us);
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
    } else if(cmd.type == CommandType::Timed) {
        motor_config.now_time = execute_timing(cmd.timing_data.ptr, cmd.timing_data.count, motor_config.now_time);
        free_timing_buffers.send(cmd.timing_data.ptr);
    } else if(cmd.type == CommandType::NewTimingReference) {
        motor_config.now_time = esp_timer_get_time();
    }
}

void flush_command_queue() {
    Command cmd;
    while(command_queue.receive(&cmd)) {
        if(cmd.type == CommandType::Timed) {
            free_timing_buffers.send(cmd.timing_data.ptr);
        }
    }
}

void setup(){
    Serial.begin(115200);
    
    for(int i : leds){
        pinMode(i, OUTPUT);
    }

    for(TimedCommand* buffer : timing_buffers) {
        free_timing_buffers.send(buffer);
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
    if(!controllers[2].init()) {
        panic(5);
    }

    control_server.on_message([](uint8_t* ptr, size_t len){
        uint8_t* head = ptr;
        uint8_t* end = ptr + len;
        while(head < end) {
            Command cmd;
            memcpy(&cmd, head, sizeof(cmd));
            head += sizeof(cmd);

            if(cmd.type == CommandType::StopNow) {
                hardware_disable_all();
                flush_command_queue();
            } else if(cmd.type == CommandType::Timed) {
                while(cmd.timing_data.count > 0) {
                    size_t chunk_size = std::min(cmd.timing_data.count, TIMING_CHUNK_SIZE);
                    if(head + chunk_size * sizeof(TimedCommand) > end) { panic(8);}

                    TimedCommand* chunk{};
                    if(!free_timing_buffers.receive(&chunk)) { panic(6); }
                    memcpy(chunk, head, chunk_size * sizeof(TimedCommand));
                    head += chunk_size * sizeof(TimedCommand);

                    Command chunk_cmd{.type = CommandType::Timed};
                    chunk_cmd.timing_data.count = chunk_size;
                    chunk_cmd.timing_data.ptr = chunk;
                    
                    if(!command_queue.send(chunk_cmd)) { panic(7); }
                    cmd.timing_data.count -= chunk_size;
                }
            } else {
                if(!command_queue.send(cmd)) { panic(8); }
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
    Command cmd;
    if(command_queue.receive(&cmd)){
        execute_cmd(cmd);
    }
    if(Serial.available()){
        int v = Serial.read();
        if(v == 'o') {
            Serial.println("Hardware enable");
            Command cmd;
            cmd.type = CommandType::StartAll;
            execute_cmd(cmd);
            cmd.type = CommandType::Enable;
            cmd.enable = {true,true,true,true};
            execute_cmd(cmd);
        } else if(v == 'l') {
            Serial.println("Hardware disable");
            Command cmd;
            cmd.type = CommandType::StopAll;
            execute_cmd(cmd);
            cmd.type = CommandType::Enable;
            cmd.enable = {false,false,false,false};
            execute_cmd(cmd);
        } else if(v == '3') {
            delay_us *= 1.1;
            Serial.print("delay = ");
            Serial.println(delay_us);
            Command cmd;
            cmd.type = CommandType::SetSpeed;
            cmd.delay_us = delay_us;
            execute_cmd(cmd);
        } else if(v == '4') {
            delay_us /= 1.1;
            Serial.print("delay = ");
            Serial.println(delay_us);
            Command cmd;
            cmd.type = CommandType::SetSpeed;
            cmd.delay_us = delay_us;
            execute_cmd(cmd);
        } else if(v == '9') {
            pos_tracker += 1600;
            Command cmd;
            Serial.print("Pos "); Serial.println(pos_tracker);
            cmd.type = CommandType::MoveTo;
            cmd.position = {pos_tracker,pos_tracker,pos_tracker,pos_tracker};
            execute_cmd(cmd);
        } else if(v == '0') {
            pos_tracker -= 1600;
            Serial.print("Pos "); Serial.println(pos_tracker);
            Command cmd;
            cmd.type = CommandType::MoveTo;
            cmd.position = {pos_tracker,pos_tracker,pos_tracker,pos_tracker};
            execute_cmd(cmd);
        }
    }
}

