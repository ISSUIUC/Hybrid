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
    NewTimingReference = 14,
    Timed = 15,
};

class TimedCommand {
public:
    TimedCommand() {}

    TimedCommand(uint8_t delay, int a, int b, int c, int d): _delay(delay) {
        _channels = (a ? 0x1 : 0x0) |
                    (b ? 0x2 : 0x0) |
                    (c ? 0x4 : 0x0) |
                    (d ? 0x8 : 0x0) |
                    (a<0?0x10 : 0x0)|
                    (b<0?0x20 : 0x0)|
                    (c<0?0x40 : 0x0)|
                    (d<0?0x80 : 0x0);
    }

    uint8_t delay() const {
        return _delay;
    }
    bool get_channel(int channel) const {
        return _channels & (0x01 << channel);
    }
    bool get_direction(int channel) const {
        return _channels & (0x10 << channel);
    }
private:
    uint8_t _delay{};
    uint8_t _channels{};
};

struct TimingData {
    size_t count;
    TimedCommand* ptr;
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
        TimingData timing_data;
    };
};
