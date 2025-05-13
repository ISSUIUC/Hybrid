// #include<Arduino.h>
// #include<Wire.h>
// #include<TMC2209.h>
// #include<USB.h>
// #include "esp_pins.h"
// #include "gcode.h"
// #include "controller.h"
// #include "queue.h"
// #include "status.h"
// #include "panic.h"
// #include "wireless.h"

// constexpr uint8_t leds[] = {
//     Pins::LED_0,
//     Pins::LED_1,
//     Pins::LED_2,
//     Pins::LED_3,
// };

// constexpr uint8_t all_pins[] = {
//     Pins::DIR_0,
//     Pins::STEP_0,
//     Pins::SDA_0,
//     Pins::ENN_0,
//     Pins::DIR_3,
//     Pins::SDA_3,
//     Pins::STEP_3,
//     Pins::ENN_3,
//     Pins::SCL,
//     Pins::DIR_1,
//     Pins::STEP_1,
//     Pins::SDA_1,
//     Pins::ENN_1,
//     Pins::DIR_2,
//     Pins::STEP_2,
//     Pins::SDA_2,
//     Pins::ENN_2,
//     Pins::SDA,
//     Pins::TP22,
//     Pins::TP23,
//     Pins::TP24,
//     Pins::TP25,
//     Pins::TP26,
//     Pins::TP27,
//     Pins::TP28,
//     Pins::TP29,
//     Pins::LED_3,
//     Pins::LED_2,
//     Pins::LED_1,
//     Pins::LED_0,
// };

// constexpr const char * all_pins_names[] = {
//     "Pins::DIR_0",
//     "Pins::STEP_0",
//     "Pins::SDA_0",
//     "Pins::ENN_0",
//     "Pins::DIR_3",
//     "Pins::SDA_3",
//     "Pins::STEP_3",
//     "Pins::ENN_3",
//     "Pins::SCL",
//     "Pins::DIR_1",
//     "Pins::STEP_1",
//     "Pins::SDA_1",
//     "Pins::ENN_1",
//     "Pins::DIR_2",
//     "Pins::STEP_2",
//     "Pins::SDA_2",
//     "Pins::ENN_2",
//     "Pins::SDA",
//     "Pins::TP22",
//     "Pins::TP23",
//     "Pins::TP24",
//     "Pins::TP25",
//     "Pins::TP26",
//     "Pins::TP27",
//     "Pins::TP28",
//     "Pins::TP29",
//     "Pins::LED_3",
//     "Pins::LED_2",
//     "Pins::LED_1",
//     "Pins::LED_0",
// };

// MotorController controllers[4] {
//     MotorController(Pins::STEP_0, Pins::DIR_0, TMC2209::SERIAL_ADDRESS_0),
//     MotorController(Pins::STEP_2, Pins::DIR_2, TMC2209::SERIAL_ADDRESS_1),
//     MotorController(Pins::STEP_3, Pins::DIR_3, TMC2209::SERIAL_ADDRESS_3),
//     MotorController(Pins::STEP_1, Pins::DIR_1, TMC2209::SERIAL_ADDRESS_2),
// };

// static uint8_t staging_buffer[16192]{};
// size_t staging_index{};
// static constexpr size_t TIMING_CHUNK_SIZE = 4096;
// static constexpr size_t TIMING_BUFFER_COUNT = 22;
// static TimedCommand timing_buffers[TIMING_BUFFER_COUNT][TIMING_CHUNK_SIZE]{};
// Queue<Command, 256> command_queue;
// Queue<TimedCommand*, TIMING_BUFFER_COUNT> free_timing_buffers;

// struct {
//     int delay_us = 2000;
//     int64_t now_time{};
//     std::array<int,4> position{};
// } motor_config;

// void hardware_enable_all() {
//     digitalWrite(Pins::ENN_0, LOW);
//     digitalWrite(Pins::ENN_1, LOW);
//     digitalWrite(Pins::ENN_2, LOW);
//     digitalWrite(Pins::ENN_3, LOW);
// }

// void hardware_disable_all() {
//     digitalWrite(Pins::ENN_0, HIGH);
//     digitalWrite(Pins::ENN_1, HIGH);
//     digitalWrite(Pins::ENN_2, HIGH);
//     digitalWrite(Pins::ENN_3, HIGH);
// }

// void delay_until_microseconds(uint64_t end) {
//     while((uint64_t)esp_timer_get_time() < end){
//         NOP();
//     }
// }

// int64_t execute_timing(const TimedCommand* cmds, size_t len, int64_t start_time) {
//     int64_t now = start_time;
//     for(int i = 0; i < len; i++){
//         TimedCommand cmd = cmds[i];
//         now += cmd.delay();
//         delay_until_microseconds(now);
//         if(now + 10 < esp_timer_get_time()) now = esp_timer_get_time();
//         for(int channel = 0; channel < 4; channel++){
//             if(cmd.get_channel(channel)) {
//                 controllers[channel].set_dir(cmd.get_direction(channel));
//                 controllers[channel].step();
//                 motor_config.position[channel] += cmd.get_direction(channel) ? 1 : -1;
//             }
//         }
//     }
//     return now;
// }

// void execute_cmd(const Command& cmd) {
//     if(cmd.type == CommandType::StopAll){
//         hardware_disable_all();
//     } else if(cmd.type == CommandType::StartAll){
//         hardware_enable_all();
//     } else if(cmd.type == CommandType::Enable){
//         for(int i = 0; i < 4; i++){
//             controllers[i].enable(cmd.enable[i]);
//         }
//     } else if(cmd.type == CommandType::SetMicrosteps){
//         for(int i = 0; i < 4; i++){
//             controllers[i].set_microsteps(cmd.microsteps[i]);
//         }
//     } else if(cmd.type == CommandType::StealthChop) {
//         for(int i = 0; i < 4; i++){
//             controllers[i].set_stealth_chop(cmd.enable[i]);
//         }
//     } else if(cmd.type == CommandType::CoolStep) {
//         for(int i = 0; i < 4; i++){
//             controllers[i].set_cool_step(cmd.enable[i]);
//         }
//     } else if(cmd.type == CommandType::Timed) {
//         motor_config.now_time = execute_timing(cmd.timing_data.ptr, cmd.timing_data.count, motor_config.now_time);
//         free_timing_buffers.send(cmd.timing_data.ptr);
//     } else if(cmd.type == CommandType::NewTimingReference) {
//         motor_config.now_time = esp_timer_get_time();
//     } else {
//         Serial.print("Unknown command "); Serial.print((int)cmd.type);
//         panic(13);
//     }
// }

// void flush_command_queue() {
//     Command cmd;
//     while(command_queue.receive(&cmd)) {
//         if(cmd.type == CommandType::Timed) {
//             free_timing_buffers.send(cmd.timing_data.ptr);
//         }
//     }
// }

// void on_command_message(uint8_t* ptr, size_t len) {
//     if(staging_index + len > sizeof(staging_buffer)) { 
//         Serial.print("Staging index "); Serial.println(staging_index);
//         Serial.print("len "); Serial.println(len);
//         panic(10); 
//     }

//     memcpy(staging_buffer + staging_index, ptr, len);
//     staging_index += len;

//     uint8_t* head = staging_buffer;
//     uint8_t* end = staging_buffer + staging_index;
//     while(end - head >= sizeof(CommandHeader)) {
//         Command cmd{};
//         memcpy(&cmd, head, Command::HEADER_SIZE);
//         if(Command::HEADER_SIZE + cmd.body_size() > end - head) {
//             break;
//         }
//         memcpy(&cmd, head, Command::HEADER_SIZE + cmd.body_size());

//         if(cmd.type == CommandType::StopNow) {
//             Serial.println("EStop");
//             hardware_disable_all();
//             flush_command_queue();
//         } else if(cmd.type == CommandType::Timed) {
//             if(cmd.full_size() > end - head) {
//                 break;
//             };
//             if(cmd.timing_data.count > TIMING_CHUNK_SIZE) { panic(11); };

//             TimedCommand* chunk{};
//             if(!free_timing_buffers.receive_wait(&chunk)) { panic(6); }

//             memcpy(chunk, head + Command::HEADER_SIZE + cmd.body_size(), cmd.timing_data.count * sizeof(TimedCommand));
//             cmd.timing_data.ptr = chunk;
//             if(!command_queue.send_wait(cmd)) { panic(7); }
//         } else {
//             if(!command_queue.send_wait(cmd)) { panic(8); }
//         }

//         head += cmd.full_size();
//     }
    
//     memmove(staging_buffer, head, end - head);
//     staging_index = end - head;
// }


// float read_temp() {
//     Wire.beginTransmission(0x44);
//     uint8_t fd = 0xfd;
//     Wire.write(&fd,1);
//     Wire.endTransmission();
//     delay(10);
//     Wire.requestFrom(0x44,3);
//     uint8_t temp_reading[2]{};
//     Wire.readBytes(temp_reading, 2);
//     float temp = -45 + 175 * (temp_reading[0] * 256.0 + temp_reading[1]) / 65535;
//     return temp;
// }

// void task_loop(void * args) {
//     delay(10);
//     Serial.print("Task loop started on core ");
//     Serial.println(xPortGetCoreID());
//     // USB.webUSB(true);
//     while(true) {
//         float t0 = read_temp();

//         Serial.print("Temp: ");
//         Serial.println(t0);
//         delay(500);
//         size_t m = millis();
//         digitalWrite(Pins::LED_0, (m / 500) % 2);
//     }
// }

// void step_loop(void * args) {
//     Serial.print("Step loop started on core ");
//     Serial.println(xPortGetCoreID());
//     while(true) {
//         Command cmd{};
//         if(command_queue.receive_wait(&cmd)){
//             execute_cmd(cmd);
//         }
//     }
// }

// void setup(){
//     Serial.begin(460800);
//     Wire.begin(Pins::SDA_1, Pins::SCL);
//     pinMode(Pins::LED_0, OUTPUT);
//     pinMode(Pins::LED_1, OUTPUT);
//     pinMode(Pins::LED_2, OUTPUT);
//     pinMode(Pins::LED_3, OUTPUT);
//     pinMode(Pins::ENN_0, OUTPUT);
//     pinMode(Pins::STEP_0, OUTPUT);
//     pinMode(Pins::DIR_0, OUTPUT);
//     pinMode(Pins::ENN_1, OUTPUT);
//     pinMode(Pins::STEP_1, OUTPUT);
//     pinMode(Pins::DIR_1, OUTPUT);
//     pinMode(Pins::ENN_2, OUTPUT);
//     pinMode(Pins::STEP_2, OUTPUT);
//     pinMode(Pins::DIR_2, OUTPUT);
//     pinMode(Pins::ENN_3, OUTPUT);
//     pinMode(Pins::STEP_3, OUTPUT);
//     pinMode(Pins::DIR_3, OUTPUT);
//     pinMode(Pins::TP22, OUTPUT);
//     pinMode(Pins::TP23, OUTPUT);
//     pinMode(Pins::TP24, OUTPUT);
//     pinMode(Pins::TP25, OUTPUT);
//     pinMode(Pins::TP26, OUTPUT);
//     pinMode(Pins::TP27, OUTPUT);
//     pinMode(Pins::TP28, OUTPUT);
//     pinMode(Pins::TP29, OUTPUT);
//     digitalWrite(Pins::ENN_0, LOW);
//     digitalWrite(Pins::STEP_0, LOW);
//     digitalWrite(Pins::DIR_0, LOW);
//     digitalWrite(Pins::ENN_1, LOW);
//     digitalWrite(Pins::STEP_1, LOW);
//     digitalWrite(Pins::DIR_1, LOW);
//     digitalWrite(Pins::ENN_2, LOW);
//     digitalWrite(Pins::STEP_2, LOW);
//     digitalWrite(Pins::DIR_2, LOW);
//     digitalWrite(Pins::ENN_3, LOW);
//     digitalWrite(Pins::STEP_3, LOW);
//     digitalWrite(Pins::DIR_3, LOW);
//     hardware_disable_all();
    
//     for(TimedCommand* buffer : timing_buffers) {
//         free_timing_buffers.send(buffer);
//     }

//     Serial0.begin(115200);
//     for(int i = 0; i < 4; i++){
//         if(!controllers[i].init()) {
//             panic(i);
//         }
//         controllers[i].enable(true);
//     }

//     hardware_disable_all();
//     digitalWrite(Pins::LED_0, HIGH);
//     digitalWrite(Pins::LED_1, HIGH);
//     digitalWrite(Pins::LED_2, HIGH);
//     digitalWrite(Pins::LED_3, HIGH);

//     // set_data_callback(on_command_message);
//     // setup_wifi("SJSK2", "srijanshukla");
//     // setup_wifi("foobar3", "25mjrn15");
//     delay(1000);

//     digitalWrite(Pins::LED_0, LOW);
//     digitalWrite(Pins::LED_1, LOW);
//     digitalWrite(Pins::LED_2, LOW);
//     digitalWrite(Pins::LED_3, LOW);
//     for(int i = 0; i < 4; i++) {
//         hardware_enable_all();
//         controllers[i].enable(true);
//     }
//     bool go = false;
//     bool dir = false;
//     while(true) {
//         if(Serial.available()) {
//             int v = Serial.read();
//             if(v == 'a') {
//                 dir = false;
//             } else if(v == 'f') {
//                 hardware_disable_all();
//             } else if(v == 'o') {
//                 hardware_enable_all();
//             } else if(v == 'd') {
//                 dir = true;
//             } else if(v == 's') {
//                 go = !go;
//             }
//         }
//         if(go) {
//             for(int i = 0; i < 4; i++) {
//                 controllers[i].set_dir(dir);
//                 controllers[i].step();
//             }
//         }
//         delay(20);
//     }

//     // xTaskCreatePinnedToCore(task_loop, "Task Loop", 8192, nullptr, 1, nullptr, 0);
//     // xTaskCreatePinnedToCore(step_loop, "Step Loop", 8192, nullptr, 1, nullptr, 1);
// }


// void loop(){
//     delay(100000);
// }

#include<Arduino.h>
// #include<TMCStepper.h>
#include<TMCDriver.h>
#include<SPI.h>

#define SPI_SCK 3
#define SPI_MOSI 2
#define SPI_MISO 1
#define EN_0 12
#define CS_0 6
#define STEP_0 4
#define DIR_0 5
#define SG_0 7
#define EN_1 42
#define CS_1 41
#define STEP_1 43
#define DIR_1 44
#define SG_1 45
#define LED0 9
#define LED1 8

TMC2660 driver(CS_0, SG_0);
TMC2660 driver1(CS_1, SG_1);
// TMC2660Stepper stepper0(CS_0, 0.075, SPI_MOSI, SPI_MISO, SPI_SCK);
// TMC2660Stepper stepper1(CS_1, 0.075, SPI_MOSI, SPI_MISO, SPI_SCK);

bool sdoff = false;

void setup() {
    Serial.begin(9600);
    SPI.begin(SPI_SCK,SPI_MISO,SPI_MOSI);
    pinMode(LED0, OUTPUT);
    pinMode(LED1, OUTPUT);
    pinMode(EN_0, OUTPUT);
    pinMode(EN_1, OUTPUT);
    pinMode(STEP_0, OUTPUT);
    pinMode(STEP_1, OUTPUT);
    pinMode(DIR_0, OUTPUT);
    pinMode(DIR_1, OUTPUT);

driver.init();
//CHOPCONF settings
driver.blankTime(24);
driver.chopperMode(1);
driver.hystEnd(5);
driver.hystStart(6);
driver.hystDecrement(32);
driver.slowDecayTime(5);

// DRVCONF settings
driver.readMode(1);
driver.senseResScale(0);
driver.stepMode(0);
driver.motorShortTimer(2);
driver.enableDetectGND(0);
driver.slopeControlLow(2);
driver.slopeControlHigh(2);

//SMARTEN settings
driver.coilLowerThreshold(8);
driver.coilIncrementSize(8);
driver.coilUpperThreshold(8);
driver.coilDecrementSpd(8);
driver.minCoilCurrent(0);

//SGCSCONF settings
driver.currentScale(25);
driver.stallGrdThresh(0);
driver.filterMode(0);
driver.setMicroStep(32);

//Write the constructed bitfields to the driver
driver.pushCommands();


driver1.init();
//CHOPCONF settings
driver1.blankTime(24);
driver1.chopperMode(1);
driver1.hystEnd(5);
driver1.hystStart(6);
driver1.hystDecrement(32);
driver1.slowDecayTime(5);

// DRVCONF settings
driver1.readMode(1);
driver1.senseResScale(0);
driver1.stepMode(0);
driver1.motorShortTimer(2);
driver1.enableDetectGND(0);
driver1.slopeControlLow(2);
driver1.slopeControlHigh(2);

//SMARTEN settings
driver1.coilLowerThreshold(8);
driver1.coilIncrementSize(8);
driver1.coilUpperThreshold(8);
driver1.coilDecrementSpd(8);
driver1.minCoilCurrent(0);

//SGCSCONF settings
driver1.currentScale(25);
driver1.stallGrdThresh(0);
driver1.filterMode(0);
driver1.setMicroStep(32);

//Write the constructed bitfields to the driver
driver1.pushCommands();


    // stepper0.begin();
    // stepper0.toff(5);
    // stepper0.microsteps(16);
    // stepper0.pwm_autoscale(true);
    // stepper0.sfilt(true);
    // stepper0.rdsel(0b01);

}


bool dir = false;
void loop() {
    for(int i = 100; i < 200; i += 1) {
        for(int b = 0; b < 50; b++) {
            digitalWrite(LED0, HIGH);
            digitalWrite(STEP_0, HIGH);
            digitalWrite(STEP_1, HIGH);
            delayMicroseconds(i*4/3);
            digitalWrite(LED0, LOW);
            digitalWrite(STEP_0, LOW);
            digitalWrite(STEP_1, LOW);
            delayMicroseconds(i*4/3);
            digitalWrite(DIR_0, !dir);
            digitalWrite(LED0, HIGH);
            digitalWrite(STEP_0, HIGH);
            digitalWrite(STEP_1, HIGH);
            delayMicroseconds(i*4/3);
            digitalWrite(LED0, LOW);
            digitalWrite(STEP_0, LOW);
            digitalWrite(STEP_1, LOW);
            delayMicroseconds(i*4/3);
            digitalWrite(DIR_0, !dir);
            dir = !dir;
        }   
    }
    
    
   
}