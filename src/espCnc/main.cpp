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
//#include<TMCStepper.h>
#include<TMCDriver.h>
#include<SPI.h>
#include <cmath>
#include <array>


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
#define BOOT 0

TMC2660 driver(CS_0, SG_0);

bool sdoff = false;

struct {
    std::array<int,2> real_position{};
    std::array<float, 2> ideal_position{};
    std::array<int,2> target_position{};
    uint64_t last_update_time = 0;
    std::array<float,2> speed{0, 0};
    std::array<float,2> max_speed{100000,100000};
    std::array<float,2> max_accel{200000,200000};
    std::array<float,2> max_decel{50000,50000};
    std::array<int,2> step_per_rev{int(4.125*32*200),int(4.667*32*200)};
} motor_config;


void step(int channel, bool dir) {
    if(channel == 0) {
        digitalWrite(DIR_0, dir);
        digitalWrite(STEP_0, HIGH);
        digitalWrite(STEP_0, LOW);
    }
    if(channel == 1 ) {
        digitalWrite(DIR_1, dir);
        digitalWrite(STEP_1, HIGH);
        digitalWrite(STEP_1, LOW);
    }   
}


void move_to_position() {
    uint64_t now = micros();
    if(motor_config.last_update_time == 0) {
        motor_config.last_update_time = now;
    }
    float dt = (now - motor_config.last_update_time) / 1000000.0;
    motor_config.last_update_time = now;

    for(int motor = 0; motor < 2; motor++) {
        //integration phase
        float& pos = motor_config.ideal_position[motor];
        float target = motor_config.target_position[motor];
        float& speed = motor_config.speed[motor];
        float accel = motor_config.max_accel[motor];
        float decel = motor_config.max_decel[motor];
        pos += speed * dt;

        //step phase
        if(pos > motor_config.real_position[motor] + 1) {
            step(motor, true);
            motor_config.real_position[motor] += 1;
            // Serial.print(pos);
            // Serial.print(' ');
            // Serial.print(speed);
            // Serial.print(' ');
            // Serial.println(dt * 1000000);
        }
        if(pos < motor_config.real_position[motor] - 1) {
            step(motor, false);
            motor_config.real_position[motor] -= 1;
            // Serial.print(pos);
            // Serial.print(' ');
            // Serial.print(speed);
            // Serial.print(' ');
            // Serial.println(dt);
        }

        //control phase
        float dir = speed > 0 ? 1 : -1;
        float decel_pos = pos + speed * speed / decel / 2.0 * dir;
        if((decel_pos - target) * dir >= 0) {
            speed -= decel * dt * dir;
        } else if(abs(speed) < motor_config.max_speed[motor]) {
            speed += accel * dt * dir;
        }
    }
}





//Below is saved for the communication between midas and the motor board.

    //WGS84 constants
constexpr double a = 6378137.0;           // Equatorial radius
constexpr double f = 1.0 / 298.257223563; // Flattening
constexpr double b = a * (1 - f);         // Polar radius
constexpr double e_sq = (a * a - b * b) / (a * a);

// Converts GPS (lat, lon, alt) to ECEF
void gps_to_ecef(double lat, double lon, double alt, double& x, double& y, double& z) {
    lat *= M_PI / 180.0;
    lon *= M_PI / 180.0;
    double N = a / std::sqrt(1 - e_sq * std::sin(lat) * std::sin(lat));
    x = (N + alt) * std::cos(lat) * std::cos(lon);
    y = (N + alt) * std::cos(lat) * std::sin(lon);
    z = ((1 - e_sq) * N + alt) * std::sin(lat);
}

// Converts ECEF to ENU
void ecef_to_enu(double x, double y, double z,
                 double ref_lat, double ref_lon, double ref_x, double ref_y, double ref_z,
                 double& east, double& north, double& up) {
    ref_lat *= M_PI / 180.0;
    ref_lon *= M_PI / 180.0;

    double dx = x - ref_x;
    double dy = y - ref_y;
    double dz = z - ref_z;

    east  = -std::sin(ref_lon) * dx + std::cos(ref_lon) * dy;
    north = -std::sin(ref_lat) * std::cos(ref_lon) * dx
            - std::sin(ref_lat) * std::sin(ref_lon) * dy
            + std::cos(ref_lat) * dz;
    up    = std::cos(ref_lat) * std::cos(ref_lon) * dx
            + std::cos(ref_lat) * std::sin(ref_lon) * dy
            + std::sin(ref_lat) * dz;
}

// Calculates pitch and yaw from ENU
void calculate_pitch_yaw(double east, double north, double up, double& pitch, double& yaw) {
    pitch = std::atan2(up, std::sqrt(east * east + north * north));
    yaw = std::atan2(east, north);
}

void update_position(double goal_lat, double goal_lon, double goal_alt,
                     double curr_lat, double curr_lon, double curr_alt) {
    double x1, y1, z1, x2, y2, z2;
    gps_to_ecef(curr_lat, curr_lon, curr_alt, x1, y1, z1);
    gps_to_ecef(goal_lat, goal_lon, goal_alt, x2, y2, z2);

    double east, north, up;
    ecef_to_enu(x2, y2, z2, curr_lat, curr_lon, x1, y1, z1, east, north, up);

    double pitch, yaw;
    calculate_pitch_yaw(east, north, up, pitch, yaw);

    // Store pitch and yaw as target motor positions (scaled or converted if needed)
    int to_go_to_pitch = static_cast<int>(pitch * 180.0 / M_PI);//Pitch in Degrees
    int to_go_to_yaw = static_cast<int>(yaw * 180.0 / M_PI);//Yaw in Degrees

    int numstepspitch = static_cast<uint32_t>(std::floor(to_go_to_pitch/.0136363636363636));
    int numstepyaw = static_cast<uint32_t>(std::floor(to_go_to_yaw/.01205357142));

    motor_config.target_position[0] = numstepspitch;
    motor_config.target_position[1] = numstepyaw;

}

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
    pinMode(CS_0, OUTPUT);
    pinMode(CS_1, OUTPUT);
    pinMode(BOOT, INPUT_PULLUP);

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
    driver.currentScale(10); //do not increase, if you do, the damn chip will get hot and de solder itself.
    //lol, turn fan on if you're going to do max current
    driver.stallGrdThresh(0);
    driver.filterMode(0);
    driver.setMicroStep(32);

    //Write the constructed bitfields to the driver
    driver.pushCommands();

    motor_config.real_position[0] = 0;
    motor_config.real_position[1] = 0;

    motor_config.target_position[0] = 0;
    motor_config.target_position[1] = 0;
}


bool validate(const std::string& number) {
    if(number.size() > 8 || number.size() == 0) return false;
    if(number[0] != '-' && (number[0] < '0' || number[0] > '9')) {
        return false;
    }
    for(int i = 1; i < number.size(); i++){
        if(number[i] < '0' || number[i] > '9') {
            return false;
        }
    }
    return true;
}

int i = 0;
std::string input_buff = "";
uint64_t last_time = 0;
void loop() {
    if(Serial.available() && i < 2) {
        char v = Serial.read();
        Serial.println(v);
        if ((v >= '0' && v <= '9') || v == '-') {
            input_buff += v;
        } else {
            if(!validate(input_buff)){
                Serial.print("Bad number ");
                Serial.println(input_buff.c_str());
            } else if (v == 'n') {
                i = (i+1)%2;
                Serial.print("Channel ");
                Serial.print(i);
                Serial.println(" Selected");
            } else if(v == 'p') {
                motor_config.target_position[i] = std::stoi(input_buff);
                Serial.print("Position Set For Channel ");
                Serial.print(i);
                Serial.print(": ");
                Serial.println(motor_config.target_position[i]);
            } else if(v == 'a') {
                motor_config.max_accel[i] = std::stoi(input_buff);
                Serial.print("Accel Set For Channel ");
                Serial.print(i);
                Serial.print(": ");
                Serial.println(motor_config.max_accel[i]);
            } else if(v == 's') {
                motor_config.max_speed[i] = std::stoi(input_buff);
                Serial.print("Speed Set For Channel ");
                Serial.print(i);
                Serial.print(": ");
                Serial.println(motor_config.max_speed[i]);
            }
            input_buff = "";
        }
    }
    move_to_position();
}