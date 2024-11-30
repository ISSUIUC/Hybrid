#include<Arduino.h>
#include<Servo.h>
#include<pins.h>

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
        digitalWrite(fire_pin, LOW);
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
AnalogInput sense_pyro_a(SENSE_PYROA_PIN, 5000, 1000);
AnalogInput sense_pyro_b(SENSE_PYROB_PIN, 5000, 1000);
AnalogInput sense_pyro_0(SENSE_PYRO0_PIN, 5000, 1000);
AnalogInput sense_pyro_1(SENSE_PYRO0_PIN, 5000, 1000);
AnalogInput sense_pt_0(SENSE_P0_PIN, 5000, 1000);
AnalogInput sense_pt_1(SENSE_P0_PIN, 5000, 1000);
AnalogInput sense_pt_2(SENSE_P0_PIN, 5000, 1000);
AnalogInput sense_pt_3(SENSE_P0_PIN, 5000, 1000);
Pyro pyro_a(ARM_PYROA_PIN, FIRE_PYROA_PIN, sense_pyro_a);
Pyro pyro_b(ARM_PYROB_PIN, FIRE_PYROB_PIN, sense_pyro_b);
Servo servo_a;
Servo servo_b;

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

    if(Serial.available()) {
        int v = Serial.read();
        if(v == 'a') {
            servo_a.write(0);
            servo_b.write(0);
            Serial.println("0");
        } else if(v == 'b'){
            servo_a.write(180);
            servo_b.write(180);
            Serial.println("180");
        } else if(v =='o'){
            digitalWrite(SERVO_ENABLE_PIN, HIGH);
            Serial.println("ON");
        } else if(v =='f'){
            digitalWrite(SERVO_ENABLE_PIN, LOW);
            Serial.println("OFF");
        }
    }
    // Serial.println(v9);
    // delay(1000);
    // pyro_a.arm();
    // pyro_a.fire();
    // Serial.print(sense_pyro_a.read());
    // Serial.print(" ");
    // Serial.println(sense_pyro_b.read());
    // delay(1000);
    // pyro_a.unfire();
    // pyro_a.disarm();
    // Serial.print(sense_pyro_a.read());
    // Serial.print(" ");
    // Serial.println(sense_pyro_b.read());
}