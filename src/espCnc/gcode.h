#pragma once

#include<array>
#include "panic.h"
#include "GCodeParser.h"

enum class GCode {
    G0RapidTravel,
    G1LinearInterpolation,
    G2CircularInterpolationClockwise,
    G3CircularInterpolationCounterClockwise,
    G4Dwell,
    G5,G6,G7,G8,G9,G10,G11,G12,G13,G14,G15,G16,
    G17XYPlaneSelection,
    G18XZPlaneSelection,
    G19YZPlaneSelection,
    G20InchMode,
    G21MetricMode,
    G22,G23,G24,G25,G26,G27,G28,G29,G30,G31,G32,
    G33,G34,G35,G36,G37,G38,G39,
    G40CancelCutterCompensation,
    G41CutterCompensationLeft,
    G42CutterCompenstaionRight,
    G43ToolLengthCompensation,
    G44,G45,G46,G47,G48,
    G49ToolLengthCompensationCancel,
    G50CancelScaling,
    G51,G52,G53,
    G54WorkOffset1,
    G55WorkOffset2,
    G56WorkOffset3,
    G57WorkOffset4,
    G58WorkOffset5,
    G59WorkOffset6,
    G60,G61,G62,G63,G64,G65,G66,G67,G68,G69,
    G70,G71,G72,
    G73HighSpeedPeckDrillingCannedCycle,
    G74LeftHandTappingCannedCycle,
    G75,
    G76FineBoringCannedCycle,
    G77,G78,G79,
    G80CannedCycleCancel,
    G81StandardDrillingCycle,
    G82StandardDrillWithDwell,
    G83DeepHolePeckDrillingCycle,
    G84RightHandTappingCycle,
    G85ReamingCycle,
    G86BoringCycle,
    G87BackBoringCycle,
    G88BoringCycleWithDwell,
    G89BackBoringCycleWithDwell,
    G90AbsoluteMode,
    G91IncrementalMode,
    G92,G93,
    G94FeedPerMinuteMode,
    G95FeedPerRevolutionMode,
    G96ConstantSurfaceSpeed,
    G97ConstantSpindleSpeed,
    G98ReturnToInitialPlane,
    G99ReturnToRapidPlane,
    Error,
};

struct GCodeCoordinate {
    float X{};
    float Y{};
    float Z{};
    float I{};
    float J{};
    float K{};
    float N{};
    uint8_t mask{};

    void print() {
        Serial.print("Coordinate: ");
        Serial.print(X); Serial.print(' ');
        Serial.print(Y); Serial.print(' ');
        Serial.print(Z); Serial.print(' ');
        Serial.print(I); Serial.print(' ');
        Serial.print(J); Serial.print(' ');
        Serial.print(K); Serial.print(' ');
        Serial.print(N); Serial.print(' ');
        Serial.println(mask);
    }

    float get_coord(int idx) {
        switch(idx) {
            case 0: return X;
            case 1: return Y;
            case 2: return Z;
            default: panic(11);
        }
    }

    bool has_coord(int idx) {
        return mask & (1 << idx);
    }

    bool has_offset(int idx) {
        return mask & (1 << (idx + 3));
    }

    float get_offset(int idx) {
        switch(idx) {
            case 0: return I;
            case 1: return J;
            case 2: return K;
            default: panic(11);
        }
    }
};

struct GCodeCommand {
    GCode code;
    GCodeCoordinate coord;
};

enum class MCode {
    M00ProgramStop,
    M01OptionalStop,
    M02EndOfProgram,
    M03SpindleOnClockwise,
    M04SpindleOnCounterClockwise,
    M05SpindleStop,
    M06ToolChange, 
    M07CoolantOnMist,
    M08CoolantOnFlood,
    M09CoolantOff,
    M10,M11,M12,M13,M14,M15,M16,M17,M18,
    M19OrientSpindle,
    M20,M21,M22,M23,M24,M25,M26,M27,M28,M29,
    M30EndOfProgram,
    M31Enable,
    M32Disable,
    Error
};

struct Command {
    char type;
    union {
        GCodeCommand gcode;
        MCode mcode;
    };
};

GCode GCode_from_number(int num);
MCode MCode_from_number(int num);
GCodeCoordinate GCode_coordinate_lex(Token* beg, Token* end);
