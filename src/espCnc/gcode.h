#pragma once

#include<array>

enum class CommandType: uint16_t {
    StopNow = 0,
    StopAll = 1,
    StartAll = 2,
    Enable = 3,
    SetMicrosteps = 4,
    SetVelocity = 5,
    MoveTo = 6,
    SetStepDir = 7,
    SetSpeed = 8,
    Wait = 9,
    ZeroPosition = 10,
    StealthChop = 11,
    CoolStep = 12,
    SetStatus = 13,
};

struct Command {
    CommandType type;
    uint16_t index;
    union {
        std::array<int,4> enable;
        std::array<int,4> set_microsteps;
        std::array<int,4> set_velocity;
        std::array<int,4> position;
        int delay_us;
        int value;
    };
};
