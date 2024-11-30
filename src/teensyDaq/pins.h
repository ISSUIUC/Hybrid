#pragma once

#include<Arduino.h>

//Pyro pins
constexpr uint8_t SENSE_PYROB_PIN = A17;
constexpr uint8_t ARM_PYROB_PIN = 40;
constexpr uint8_t FIRE_PYROB_PIN = 39;
constexpr uint8_t SENSE_PYROA_PIN = A14;
constexpr uint8_t ARM_PYROA_PIN = 37;
constexpr uint8_t FIRE_PYROA_PIN = 36;

//Top side voltage sense pins
constexpr uint8_t SENSE_BATT_PIN = A13;
constexpr uint8_t SENSE_5V_PIN = A12;
constexpr uint8_t SENSE_6VA_PIN = A11;
constexpr uint8_t SENSE_6VB_PIN = A10;

//Load cell pins
constexpr uint8_t LOAD_CELL_DAT = 23;
constexpr uint8_t LOAD_CELL_CLK = 22;

//Bottom side voltage sense pins
constexpr uint8_t SENSE_9V_PIN = A7;
constexpr uint8_t SENSE_P3_PIN = A6;
constexpr uint8_t SENSE_P2_PIN = A5;
constexpr uint8_t SENSE_P1_PIN = A4;
constexpr uint8_t SENSE_P0_PIN = A3;
constexpr uint8_t SENSE_PYRO0_PIN = A2;
constexpr uint8_t SENSE_PYRO1_PIN = A1;
constexpr uint8_t SENSE_VPYRO_PIN = A0;

//Led pins
constexpr uint8_t LED0_PIN = 0;
constexpr uint8_t LED1_PIN = 1;
constexpr uint8_t LED2_PIN = 2;
constexpr uint8_t LED3_PIN = 3;

//Servo pins
constexpr uint8_t SERVO_ENABLE_PIN = 30;
constexpr uint8_t SERVO_A_PWM = 29;
constexpr uint8_t SERVO_B_PWM = 28;
