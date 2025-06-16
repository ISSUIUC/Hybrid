#pragma once
#include "controller.h"
#include "GCode.h"

enum class CoordinateMode {
    Absolute,
    Incremental,
};

enum class PlaneReferenceMode {
    XY,
    XZ,
    YZ,
};

struct PathExecutor {
public:
    PathExecutor(MotorController* motors):
    mode(CoordinateMode::Absolute), motors(motors) {

    }

    void execute(GCodeCommand cmd);

private:
    void fast_linear_step(int dx, int dy, int dz);
    void swizzle_linear_step(int* d_pos, int* rate, int ax0, int ax1, int ax2);
    void linear_step(int dx, int dy, int dz, int xrate, int yrate, int zrate);
    void circular_arc(bool counterclockwise, GCodeCoordinate coord);
    float get_pos(int idx) {
        switch(idx) {
            case 0: return X;
            case 1: return Y;
            case 2: return Z;
            default: panic(10);
        }
    }
    void set_pos(int idx, float v) {
        switch(idx) {
            case 0: X = v; break;
            case 1: Y = v; break;
            case 2: Z = v; break;
            default: panic(10);
        }
    }
    CoordinateMode mode;
    PlaneReferenceMode plane_mode = PlaneReferenceMode::XY;
    float X{},Y{},Z{};
    MotorController* motors;
};