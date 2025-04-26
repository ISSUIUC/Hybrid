#include<Servo.h>
#include"pins.h"
#include<Arduino.h>
#include<HX711.h>

class AnalogInput {
public:
    AnalogInput(uint8_t pin, float R1, float R2): pin(pin), R1(R1), R2(R2) {
        pinMode(pin, INPUT);
    }

    float read() {
        int raw = analogRead(pin);

        return raw * raw_factor * (R1 + R2) / R2;
    }

private:
    static constexpr float raw_factor = 3.3/1024.0;
    uint8_t pin;
    float R1;
    float R2;
};

class Pyro {
public:
    Pyro(uint8_t arm_pin, uint8_t fire_pin, AnalogInput& sense_pin): arm_pin(arm_pin), fire_pin(fire_pin), sense_pin(sense_pin) {
        pinMode(arm_pin, OUTPUT);
        digitalWrite(arm_pin, LOW);
        pinMode(fire_pin, OUTPUT);
        digitalWrite
        (fire_pin, LOW);
    }

    void arm() {
        digitalWrite(arm_pin, HIGH);
    }

    void disarm() {
        digitalWrite(arm_pin, LOW);
    }

    void fire() {
        digitalWrite(fire_pin, HIGH);
    }

    void unfire() {
        digitalWrite(fire_pin, LOW);
    }

private:

    uint8_t arm_pin;
    uint8_t fire_pin;
    AnalogInput& sense_pin;
};

AnalogInput sense_batt(SENSE_BATT_PIN, 5000, 1000);
AnalogInput sense_5v(SENSE_5V_PIN, 5000, 1000);
AnalogInput sense_9v(SENSE_9V_PIN, 5000, 1000);
AnalogInput sense_6va(SENSE_6VA_PIN, 5000, 1000);
AnalogInput sense_6vb(SENSE_6VB_PIN, 5000, 1000);
AnalogInput sense_pyro_a(SENSE_PYROA_PIN, 49900, 5000);
AnalogInput sense_pyro_b(SENSE_PYROB_PIN, 5000, 1000);
AnalogInput sense_pyro_0(SENSE_PYRO0_PIN, 5000, 1000);
AnalogInput sense_pyro_1(SENSE_PYRO1_PIN, 5000, 1000);
AnalogInput sense_pt_0(SENSE_P0_PIN, 5000, 1000);
AnalogInput sense_pt_1(SENSE_P1_PIN, 5000, 1000);
AnalogInput sense_pt_2(SENSE_P2_PIN, 5000, 1000);
AnalogInput sense_pt_3(SENSE_P3_PIN, 5000, 1000);
Pyro pyro_a(ARM_PYROA_PIN, FIRE_PYROA_PIN, sense_pyro_a);
Pyro pyro_b(ARM_PYROB_PIN, FIRE_PYROB_PIN, sense_pyro_b);
Servo servo_a;
Servo servo_b;
HX711 scale;
#define BALLVALVE_OPEN_ANGLE 0
#define BALLVAVLE_CLOSE_ANGLE 150
#define FIRE_DELAY 3500

float scale_reading = -1.0;
uint32_t fire_time = 0;
bool engage_ballvalve = false;
bool armed = false;
bool ballvalve_open = false;

void open_ballvalve() {
    servo_a.write(BALLVALVE_OPEN_ANGLE);
    servo_b.write(BALLVALVE_OPEN_ANGLE);
    ballvalve_open = true;
}

void close_ballvalve() {
    servo_a.write(BALLVAVLE_CLOSE_ANGLE);
    servo_b.write(BALLVAVLE_CLOSE_ANGLE);
    ballvalve_open = false;
}

void arm() {
    pyro_a.arm();
    pyro_b.arm();
    armed = true;
}

void disarm() {
    pyro_a.disarm();
    pyro_b.disarm();
    armed = false;
    engage_ballvalve = false;
}

void fire() {
    pyro_a.fire();
    pyro_b.fire();
    fire_time = millis();
    engage_ballvalve = true;
    delay(10);
    pyro_a.unfire();
    pyro_b.unfire();
    pyro_a.disarm();
    pyro_b.disarm();
    armed = false;
}

void setup() {
    Serial.begin(115200);
    while(!Serial);
    pinMode(LED0_PIN, OUTPUT);
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    pinMode(LED3_PIN, OUTPUT);
    pinMode(SERVO_ENABLE_PIN, OUTPUT);
    servo_a.attach(SERVO_A_PWM);
    servo_b.attach(SERVO_B_PWM);
    close_ballvalve();
    scale.begin(22, 23);
    scale.set_scale(2886.5);
    delay(500);
    if(scale.is_ready()){
        scale.tare();
    }
}


void loop() {
    float v9 = sense_9v.read();
    float v5 = sense_5v.read();
    float vbatt = sense_batt.read();

    bool batt_good = vbatt > 11.0;
    bool v9_good = 8.95 < v9 && v9 < 9.05;
    bool v5_good = 4.95 < v5 && v5 < 5.05;
    digitalWrite(LED0_PIN, batt_good);
    digitalWrite(LED1_PIN, v9_good);
    digitalWrite(LED2_PIN, v5_good);
    digitalWrite(LED3_PIN, true);

    while (Serial.available())
    {
        int serialVal = Serial.read();
        if(serialVal == 'c') {
            close_ballvalve();
        } else if(serialVal == 'o') {
            open_ballvalve();
        } else if(serialVal == 'a') {
            arm();
        } else if(serialVal == 'd') {
            disarm();
        } else if(serialVal == 'f') {
            fire();
        }
    }

    if(engage_ballvalve && (millis() > fire_time + FIRE_DELAY)) {
        open_ballvalve();
        engage_ballvalve = false;
    }

    float a = 3.33261;
    float b = -388.278;
    //Sense pt0-4
    float voltPT0 = (sense_pt_0.read()*682.518528)+b;
    float voltPT1 = (sense_pt_1.read()*682.518528)+b;
    float voltPT2 = (sense_pt_2.read()*682.518528)+b;
    float voltPT3 = (sense_pt_3.read()*682.518528)+b;
    float voltSenseA = sense_pyro_a.read();
    float voltSenseB = sense_pyro_b.read();
    float voltPyroIn0 = sense_pyro_0.read();
    float voltPyroIn1 = sense_pyro_1.read();

    if(scale.is_ready()) {
        scale_reading = scale.get_units();
    } 

    Serial.print(millis());
    Serial.print('\t');
    Serial.print(voltPT0);
    Serial.print('\t');
    Serial.print(voltPT1);
    Serial.print('\t');
    Serial.print(voltPT2);
    Serial.print('\t');
    Serial.print(voltPT3);
    Serial.print('\t');
    Serial.print(voltSenseA);
    Serial.print('\t');
    Serial.print(voltSenseB);
    Serial.print('\t');
    Serial.print(voltPyroIn0);
    Serial.print('\t');
    Serial.print(voltPyroIn1);
    Serial.print('\t');
    Serial.print(armed);
    Serial.print('\t');
    Serial.print(ballvalve_open);
    Serial.print('\t');
    Serial.print(engage_ballvalve);
    Serial.print('\t');
    Serial.println(scale_reading);
    delay(10);
}