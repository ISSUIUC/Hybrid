#include<Arduino.h>
#include<Wire.h>
#include<TMC2209.h>
#include<USB.h>
#include "esp_pins.h"
#include "gcode.h"
#include "controller.h"
#include "queue.h"
#include "panic.h"
#include "GCodeParser.h"
#include "PathExecute.h"

Queue<Command, 32> queued_commands;

MotorController controllers[4] {
    MotorController(Pins::STEP_0, Pins::DIR_0, TMC2209::SERIAL_ADDRESS_0, 40, 20), //XAxis
    MotorController(Pins::STEP_3, Pins::DIR_3, TMC2209::SERIAL_ADDRESS_3, 8,  20), //YAxis
    MotorController(Pins::STEP_1, Pins::DIR_1, TMC2209::SERIAL_ADDRESS_2, 8,  5), //ZAxis
    MotorController(Pins::STEP_2, Pins::DIR_2, TMC2209::SERIAL_ADDRESS_1, 8,  20), //AAxis
};

void hardware_enable_all() {
    digitalWrite(Pins::ENN_0, LOW);
    digitalWrite(Pins::ENN_1, LOW);
    digitalWrite(Pins::ENN_2, LOW);
    digitalWrite(Pins::ENN_3, LOW);
    digitalWrite(Pins::FanControl, HIGH);
    for(MotorController& controller: controllers) {
        controller.init();
        controller.enable(true);
    }
}

void hardware_disable_all() {
    digitalWrite(Pins::ENN_0, HIGH);
    digitalWrite(Pins::ENN_1, HIGH);
    digitalWrite(Pins::ENN_2, HIGH);
    digitalWrite(Pins::ENN_3, HIGH);
    digitalWrite(Pins::FanControl, LOW);
}

void delay_until_microseconds(uint64_t end) {
    while((uint64_t)esp_timer_get_time() < end){
        NOP();
    }
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

void queue_command(Token primary, Token* params_begin, Token* params_end) {
    Command cmd{};
    if(primary.letter == 'M') {
        cmd.type = 'M';
        cmd.mcode = MCode_from_number(primary.number);
    }
    if(primary.letter == 'G') {
        cmd.type = 'G';
        cmd.gcode.code = GCode_from_number(primary.number);
        cmd.gcode.coord = GCode_coordinate_lex(params_begin, params_end);
    }
    queued_commands.send_wait(cmd);
}

void task_loop(void * args) {
    delay(10);
    Serial.print("Task loop started on core ");
    Serial.println(xPortGetCoreID());
    GCodeParser parser{};
    parser.set_callback(queue_command);

    while(true) {
        while(Serial.available()) {
            int v = Serial.read();
            Serial.print('%');
            parser.next_char(v);
        }
        uint32_t time = millis();
        if(time % 500 == 0) {
            float t0 = read_temp();
            Serial.print("Temp: ");
            Serial.println(t0);
        }
        
        delay(1);
        // digitalWrite(Pins::LED_0, (m / 500) % 2);
    }
}

void step_loop(void * args) {
    Serial.print("Step loop started on core ");
    Serial.println(xPortGetCoreID());

    PathExecutor path_executor(controllers);
    while(true) {
        Command cmd{};
        if(queued_commands.receive_wait(&cmd)) {
            if(cmd.type == 'G') {
                path_executor.execute(cmd.gcode);
            }
            if(cmd.type == 'M') {
                MCode code = cmd.mcode;
                if(code == MCode::M31Enable) {
                    hardware_enable_all();
                }
                if(code == MCode::M32Disable) {
                    hardware_disable_all();
                }
            }
        }
    }
}

void setup(){
    Serial.begin(460800);
    USB.webUSB(true);
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
    pinMode(Pins::FanControl, OUTPUT);
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
    hardware_enable_all();
    
    Serial0.begin(115200);
    for(int i = 0; i < 4; i++){
        if(!controllers[i].init()) {
            panic(i);
        }
    }

    hardware_disable_all();
    digitalWrite(Pins::LED_0, HIGH);
    digitalWrite(Pins::LED_1, HIGH);
    digitalWrite(Pins::LED_2, HIGH);
    digitalWrite(Pins::LED_3, HIGH);
    
    xTaskCreatePinnedToCore(task_loop, "Task Loop", 8192, nullptr, 1, nullptr, 0);
    xTaskCreatePinnedToCore(step_loop, "Step Loop", 8192, nullptr, 1, nullptr, 1);
}


void loop(){
    delay(100000);
}

