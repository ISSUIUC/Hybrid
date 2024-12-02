#pragma once

#include<array>
#include "panic.h"

enum class CommandType: uint16_t {
    StopNow = 0,
    StopAll = 1,
    StartAll = 2,
    Enable = 3,
    SetMicrosteps = 4,
    StealthChop = 11,
    CoolStep = 12,
    NewTimingReference = 14,
    Timed = 15,
};

struct CommandHeader {
    CommandType type;
    uint16_t index;

    
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
        return (_channels & (0x01 << channel)) != 0;
    }
    bool get_direction(int channel) const {
        return (_channels & (0x10 << channel)) != 0;
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
        std::array<bool,4> enable;
        std::array<uint16_t,4> microsteps;
        TimingData timing_data;
    };

    static constexpr size_t HEADER_SIZE = 4;

    size_t body_size() {
        switch(type){
            case CommandType::StopNow:
                return 0;
            case CommandType::StopAll:
                return 0;
            case CommandType::StartAll:
                return 0;
            case CommandType::Enable:
                return sizeof(Command::enable);
            case CommandType::SetMicrosteps:
                return sizeof(Command::microsteps);
            case CommandType::StealthChop:
                return sizeof(Command::enable);
            case CommandType::CoolStep:
                return sizeof(Command::enable);
            case CommandType::NewTimingReference:
                return 0;
            case CommandType::Timed:
                return sizeof(TimingData::count);
            default:
                panic(14);
                return 0;
        }
    }

    size_t dynamic_size() {
        if(type == CommandType::Timed) {
            return timing_data.count * sizeof(TimedCommand);
        } else {
            return 0;
        }
    }

    size_t full_size() {
        return HEADER_SIZE + body_size() + dynamic_size();
    }
};
