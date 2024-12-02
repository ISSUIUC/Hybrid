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
#include "panic.h"

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
static uint8_t staging_buffer[16192]{};
size_t staging_index{};
static constexpr size_t TIMING_CHUNK_SIZE = 4096;
static constexpr size_t TIMING_BUFFER_COUNT = 22;
static TimedCommand timing_buffers[TIMING_BUFFER_COUNT][TIMING_CHUNK_SIZE]{};
WirelessServer control_server{};
Queue<Command, 256> command_queue;
Queue<TimedCommand*, TIMING_BUFFER_COUNT> free_timing_buffers;

struct {
    int delay_us = 2000;
    int64_t now_time{};
    std::array<int,4> position{};
} motor_config;

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
            controllers[i].set_microsteps(cmd.microsteps[i]);
        }
    } else if(cmd.type == CommandType::StealthChop) {
        for(int i = 0; i < 4; i++){
            controllers[i].set_stealth_chop(cmd.enable[i]);
        }
    } else if(cmd.type == CommandType::CoolStep) {
        for(int i = 0; i < 4; i++){
            controllers[i].set_cool_step(cmd.enable[i]);
        }
    } else if(cmd.type == CommandType::Timed) {
        motor_config.now_time = execute_timing(cmd.timing_data.ptr, cmd.timing_data.count, motor_config.now_time);
        free_timing_buffers.send(cmd.timing_data.ptr);
    } else if(cmd.type == CommandType::NewTimingReference) {
        motor_config.now_time = esp_timer_get_time();
    } else {
        Serial.print("Unknown command "); Serial.print((int)cmd.type);
        panic(13);
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
        if(staging_index + len > sizeof(staging_buffer)) { 
            Serial.print("Staging index "); Serial.println(staging_index);
            Serial.print("len "); Serial.println(len);
            panic(10); 
        }

        memcpy(staging_buffer + staging_index, ptr, len);
        staging_index += len;

        uint8_t* head = staging_buffer;
        uint8_t* end = staging_buffer + staging_index;
        while(end - head >= sizeof(CommandHeader)) {
            Command cmd{};
            memcpy(&cmd, head, Command::HEADER_SIZE);
            if(Command::HEADER_SIZE + cmd.body_size() > end - head) {
                break;
            }
            memcpy(&cmd, head, Command::HEADER_SIZE + cmd.body_size());

            if(cmd.type == CommandType::StopNow) {
                hardware_disable_all();
                flush_command_queue();
            } else if(cmd.type == CommandType::Timed) {
                if(cmd.full_size() > end - head) {
                    break;
                };
                if(cmd.timing_data.count > TIMING_CHUNK_SIZE) { panic(11); };

                TimedCommand* chunk{};
                if(!free_timing_buffers.receive_wait(&chunk)) { panic(6); }

                memcpy(chunk, head + Command::HEADER_SIZE + cmd.body_size(), cmd.timing_data.count * sizeof(TimedCommand));
                cmd.timing_data.ptr = chunk;
                if(!command_queue.send_wait(cmd)) { panic(7); }
            } else {
                if(!command_queue.send_wait(cmd)) { panic(8); }
            }

            head += cmd.full_size();
        }
        
        memmove(staging_buffer, head, end - head);
        staging_index = end - head;
    });

    control_server.setup_wifi("SJSK2","srijanshukla");
    hardware_enable_all();
    digitalWrite(Pins::LED_0, HIGH);
}

int delay_us = 2000;
int pos_tracker = 0;

void loop(){
    Command cmd;
    if(command_queue.receive(&cmd)){
        execute_cmd(cmd);
        control_server.set_status(Status{.instruction_number = cmd.index});
    }
}

