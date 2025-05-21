#include "Gcode.h"

GCode GCode_from_number(int num) {
    if(num >= (int)GCode::Error) {
        panic(13);
        return GCode::Error;
    } else {
        return (GCode)num;
    }
}

MCode MCode_from_number(int num) {
    if(num >= (int)MCode::Error) {
        panic(12);
        return MCode::Error;
    } else {
        return (MCode)num;
    }
}


GCodeCoordinate GCode_coordinate_lex(Token* beg, Token* end) {
    GCodeCoordinate coord;
    for(Token* t = beg; t != end; t++) {
        switch(t->letter) {
            case 'X':
                coord.X = t->number;
                coord.mask |= (1 << 0);
                break;
            case 'Y':
                coord.Y = t->number;
                coord.mask |= (1 << 1);
                break;
            case 'Z':
                coord.Z = t->number;
                coord.mask |= (1 << 2);
                break;
            case 'I':
                coord.I = t->number;
                coord.mask |= (1 << 3);
                break;
            case 'J':
                coord.J = t->number;
                coord.mask |= (1 << 4);
                break;
            default:
                panic(14);
        }
    }
    return coord;
}