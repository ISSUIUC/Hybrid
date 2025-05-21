#pragma once
#include "controller.h"
#include "GCode.h"

enum class CoordinateMode {
    Absolute,
    Incremental,
};

struct PathExecutor {
public:
    PathExecutor(MotorController* motors):
    mode(CoordinateMode::Absolute), motors(motors) {

    }

    void execute(GCodeCommand cmd);

private:
    void linear_step(int dx, int dy, int dz, int xrate, int yrate, int zrate);
    CoordinateMode mode;
    float X{},Y{},Z{};
    MotorController* motors;
};