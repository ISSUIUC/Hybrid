#include <Arduino.h>
#include <Wire.h>
#include <TMC2209.h>
#include <USB.h>
#include "esp_pins.h"
#include "gcode.h"
#include "controller.h"
#include "queue.h"
#include "status.h"
#include "panic.h"
#include "wireless.h"
#include <string>

constexpr uint8_t leds[] = {
    Pins::LED_0,
    Pins::LED_1,
    Pins::LED_2,
    Pins::LED_3,
};

constexpr uint8_t all_pins[] = {
    Pins::DIR_0,
    Pins::STEP_0,
    Pins::SDA_0,
    Pins::ENN_0,
    Pins::DIR_3,
    Pins::SDA_3,
    Pins::STEP_3,
    Pins::ENN_3,
    Pins::SCL,
    Pins::DIR_1,
    Pins::STEP_1,
    Pins::SDA_1,
    Pins::ENN_1,
    Pins::DIR_2,
    Pins::STEP_2,
    Pins::SDA_2,
    Pins::ENN_2,
    Pins::SDA,
    Pins::TP22,
    Pins::TP23,
    Pins::TP24,
    Pins::TP25,
    Pins::TP26,
    Pins::TP27,
    Pins::TP28,
    Pins::TP29,
    Pins::LED_3,
    Pins::LED_2,
    Pins::LED_1,
    Pins::LED_0,
};

constexpr const char * all_pins_names[] = {
    "Pins::DIR_0",
    "Pins::STEP_0",
    "Pins::SDA_0",
    "Pins::ENN_0",
    "Pins::DIR_3",
    "Pins::SDA_3",
    "Pins::STEP_3",
    "Pins::ENN_3",
    "Pins::SCL",
    "Pins::DIR_1",
    "Pins::STEP_1",
    "Pins::SDA_1",
    "Pins::ENN_1",
    "Pins::DIR_2",
    "Pins::STEP_2",
    "Pins::SDA_2",
    "Pins::ENN_2",
    "Pins::SDA",
    "Pins::TP22",
    "Pins::TP23",
    "Pins::TP24",
    "Pins::TP25",
    "Pins::TP26",
    "Pins::TP27",
    "Pins::TP28",
    "Pins::TP29",
    "Pins::LED_3",
    "Pins::LED_2",
    "Pins::LED_1",
    "Pins::LED_0",
};

MotorController controllers[4] {
    MotorController(Pins::STEP_0, Pins::DIR_0, TMC2209::SERIAL_ADDRESS_0),
    MotorController(Pins::STEP_2, Pins::DIR_2, TMC2209::SERIAL_ADDRESS_1),
    MotorController(Pins::STEP_3, Pins::DIR_3, TMC2209::SERIAL_ADDRESS_3),
    MotorController(Pins::STEP_1, Pins::DIR_1, TMC2209::SERIAL_ADDRESS_2),
};

static uint8_t staging_buffer[16192]{};
size_t staging_index{};
static constexpr size_t TIMING_CHUNK_SIZE = 4096;
static constexpr size_t TIMING_BUFFER_COUNT = 22;
static TimedCommand timing_buffers[TIMING_BUFFER_COUNT][TIMING_CHUNK_SIZE]{};
Queue<Command, 256> command_queue;
Queue<TimedCommand*, TIMING_BUFFER_COUNT> free_timing_buffers;

struct {
    int delay_us = 2000;
    int64_t now_time{};
    std::array<int,4> position{};
} motor_config;

void hardware_enable_all() {
    digitalWrite(Pins::ENN_0, LOW);
    digitalWrite(Pins::ENN_1, LOW);
    digitalWrite(Pins::ENN_2, LOW);
    digitalWrite(Pins::ENN_3, LOW);
}

void hardware_disable_all() {
    digitalWrite(Pins::ENN_0, HIGH);
    digitalWrite(Pins::ENN_1, HIGH);
    digitalWrite(Pins::ENN_2, HIGH);
    digitalWrite(Pins::ENN_3, HIGH);
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
        if(now + 10 < esp_timer_get_time()) now = esp_timer_get_time();
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

void on_command_message(uint8_t* ptr, size_t len) {
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
            Serial.println("EStop");
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
}


float read_temp() {
    Wire.beginTransmission(0x44);
    uint8_t fd = 0xfd;
    Wire.write(&fd,1);
    Wire.endTransmission();
    delay(10);
    Wire.requestFrom(0x44,3);
    uint8_t temp_reading[2]{};
    Wire.readBytes(temp_reading, 2);
    float temp = -45 + 175 * (temp_reading[0] * 256.0 + temp_reading[1]) / 65535;
    return temp;
}

void task_loop(void * args) {
    delay(10);
    Serial.print("Task loop started on core ");
    Serial.println(xPortGetCoreID());
    // USB.webUSB(true);
    while(true) {
        float t0 = read_temp();

        Serial.print("Temp: ");
        Serial.println(t0);
        delay(500);
        size_t m = millis();
        digitalWrite(Pins::LED_0, (m / 500) % 2);
    }
}

void step_loop(void * args) {
    Serial.print("Step loop started on core ");
    Serial.println(xPortGetCoreID());
    while(true) {
        Command cmd{};
        if(command_queue.receive_wait(&cmd)){
            execute_cmd(cmd);
        }
    }
}

void setup(){
    Serial.begin(460800);
    Wire.begin(Pins::SDA_1, Pins::SCL);
    pinMode(Pins::LED_0, OUTPUT);
    pinMode(Pins::LED_1, OUTPUT);
    pinMode(Pins::LED_2, OUTPUT);
    pinMode(Pins::LED_3, OUTPUT);
    pinMode(Pins::ENN_0, OUTPUT);
    pinMode(Pins::STEP_0, OUTPUT);
    pinMode(Pins::DIR_0, OUTPUT);
    pinMode(Pins::ENN_1, OUTPUT);
    pinMode(Pins::STEP_1, OUTPUT);
    pinMode(Pins::DIR_1, OUTPUT);
    pinMode(Pins::ENN_2, OUTPUT);
    pinMode(Pins::STEP_2, OUTPUT);
    pinMode(Pins::DIR_2, OUTPUT);
    pinMode(Pins::ENN_3, OUTPUT);
    pinMode(Pins::STEP_3, OUTPUT);
    pinMode(Pins::DIR_3, OUTPUT);
    pinMode(Pins::TP22, OUTPUT);
    pinMode(Pins::TP23, OUTPUT);
    pinMode(Pins::TP24, OUTPUT);
    pinMode(Pins::TP25, OUTPUT);
    pinMode(Pins::TP26, OUTPUT);
    pinMode(Pins::TP27, OUTPUT);
    pinMode(Pins::TP28, OUTPUT);
    pinMode(Pins::TP29, OUTPUT);
    digitalWrite(Pins::ENN_0, LOW);
    digitalWrite(Pins::STEP_0, LOW);
    digitalWrite(Pins::DIR_0, LOW);
    digitalWrite(Pins::ENN_1, LOW);
    digitalWrite(Pins::STEP_1, LOW);
    digitalWrite(Pins::DIR_1, LOW);
    digitalWrite(Pins::ENN_2, LOW);
    digitalWrite(Pins::STEP_2, LOW);
    digitalWrite(Pins::DIR_2, LOW);
    digitalWrite(Pins::ENN_3, LOW);
    digitalWrite(Pins::STEP_3, LOW);
    digitalWrite(Pins::DIR_3, LOW);
    hardware_disable_all();
    
    // for(TimedCommand* buffer : timing_buffers) {
    //     free_timing_buffers.send(buffer);
    // }

    Serial0.begin(115200);
    for(int i = 0; i < 4; i++){
        if(!controllers[i].init()) {
            panic(i);
        }
        controllers[i].enable(true);
    }

    hardware_disable_all();
    digitalWrite(Pins::LED_0, HIGH);
    digitalWrite(Pins::LED_1, HIGH);
    digitalWrite(Pins::LED_2, HIGH);
    digitalWrite(Pins::LED_3, HIGH);

    // set_data_callback(on_command_message);
    // setup_wifi("SJSK2", "srijanshukla");
    // setup_wifi("foobar3", "25mjrn15");
    delay(1000);

    digitalWrite(Pins::LED_0, LOW);
    digitalWrite(Pins::LED_1, LOW);
    digitalWrite(Pins::LED_2, LOW);
    digitalWrite(Pins::LED_3, LOW);
    for(int i = 0; i < 4; i++) {
        hardware_enable_all();
        controllers[i].enable(true);
    }
    bool go = false;
    bool dir = false;
    int goal = 0;
    int curr = 0;
    std::string readGoal;
    while(true) {
        if(Serial.available()) {
            int v = Serial.read();
            if (isdigit(v)) {
                readGoal += v;
                Serial.println(static_cast<int>(v));
            } else if (v == '\n' || v == '\r') {
                Serial.println(readGoal.c_str());
                goal = std::stoi(readGoal);
                Serial.println(goal);
                readGoal = "";
                Serial.println("hello1");
                dir = (goal > curr);
                // Serial.println("Dir: " + dir);
                go = true;
                Serial.println("hello2");
            } else if(v == 'a') {
                dir = false;
            } else if(v == 'f') {
                hardware_disable_all();
            } else if(v == 'o') {
                hardware_enable_all();
            } else if(v == 'd') {
                dir = true;
            } else if(v == 's') {
                go = !go;
            }
        }
        if(go) {
            Serial.println("Hello3");
            for(int i = 0; i < 4; i++) {
                controllers[i].set_dir(dir);
                controllers[i].step();
            }
            dir ? curr++ : curr--;
            if (curr == goal) go = false;
            Serial.println("Hello4");
        }
        delay(20);
    }

    // xTaskCreatePinnedToCore(task_loop, "Task Loop", 8192, nullptr, 1, nullptr, 0);
    // xTaskCreatePinnedToCore(step_loop, "Step Loop", 8192, nullptr, 1, nullptr, 1);
}


void loop(){
    delay(100000);
}

