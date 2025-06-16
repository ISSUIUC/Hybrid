#include<Arduino.h>
#include<SPI.h>

#define MOSI 12
#define SCK 13
#define CS 14
#define LED_BLUE 15
#define LED_GREEN 16
#define LED_ORANGE 17
#define LED_RED 18
#define I0 3
#define I1 4
#define Q0 5
#define Q1 6
#define CLKOUT 7

enum class MaxRegister: uint32_t {
    CONF1 = 0,
    CONF2 = 1,
    CONF3 = 2,
    PLLCONF = 3,
    DIV = 4,
    FDIV = 5,
    STRM = 6,
    CLK = 7,
    TEST1 = 8,
    TEST2 = 9,
};

void write_reg(MaxRegister reg, uint32_t config) {
    uint32_t packed = (config << 4) | ((uint32_t)reg & 0xf);
    SPISettings settings{};
    digitalWrite(CS, LOW);
    SPI.beginTransaction(settings);
    SPI.write32(packed);
    SPI.endTransaction();
    digitalWrite(CS, HIGH);
}

struct Configuration1 {
    uint32_t CHIPEN = 1;
    uint32_t IDLE = 0;
    uint32_t ILNA1 = 0b1000;
    uint32_t ILNA2 = 0b10;
    uint32_t ILO = 0b10;
    uint32_t IMIX = 0b01;
    uint32_t MIXPOLE = 0;
    uint32_t LNAMODE = 0b00;
    uint32_t MIXEN = 1;
    uint32_t ANTEN = 1;
    uint32_t FCEN = 0b00110;
    uint32_t FBW = 0b00;
    uint32_t F3OR5 = 0;
    uint32_t FCENX = 1;
    uint32_t FGAIN = 1;

    uint32_t encode() {
        return (CHIPEN << 27)
        | (IDLE << 26)
        | (ILNA1 << 22)
        | (ILNA2 << 20)
        | (ILO << 18)
        | (IMIX << 16)
        | (MIXPOLE << 15)
        | (LNAMODE << 13)
        | (MIXEN << 12) 
        | (ANTEN << 11)
        | (FCEN << 5)
        | (FBW << 3)
        | (F3OR5 << 2)
        | (FCENX << 1)
        | (FGAIN << 0);
    }
};

struct Configuration2 {
    uint32_t IQEN = 0;
    uint32_t GAINREF = 170;
    uint32_t AGCMODE = 0b00;
    uint32_t FORMAT = 0b01;
    uint32_t BITS = 0b010;
    uint32_t DRVCFG = 0b00;
    uint32_t LOEN = 1;

    uint32_t encode() {
        return (IQEN << 27)
        | (GAINREF << 15)
        | (AGCMODE << 11)
        | (FORMAT << 9)
        | (BITS << 6)
        | (DRVCFG << 4)
        | (LOEN << 3);
    }
};

struct Configuration3 {
    uint32_t GAININ = 0b111010;
    uint32_t FSLOWEN = 1;
    uint32_t HILOADEN = 0;
    uint32_t ADCEN = 1;
    uint32_t DRVEN = 1;
    uint32_t FOFSTEN = 1;
    uint32_t FILTEN = 1;
    uint32_t FHIPEN = 1;
    uint32_t PGAQEN = 0;
    uint32_t PGAIEN = 1;
    uint32_t STRMEN = 0;
    uint32_t STRMSTART = 0;
    uint32_t STRMSTOP = 0;
    uint32_t STRMCOUNT = 0b111;
    uint32_t STRMBITS = 0b01;
    uint32_t STAMPEN = 1;
    uint32_t TIMESYNCEN = 1;
    uint32_t DATASYNCEN = 0;
    uint32_t SSTRMRST = 0;

    uint32_t encode() {
    return (GAININ << 22)
    | (FSLOWEN << 21)
    | (HILOADEN << 20)
    | (ADCEN << 19)
    | (DRVEN << 18)
    | (FOFSTEN << 17)
    | (FILTEN << 16)
    | (FHIPEN << 15)
    | (1 << 14)
    | (PGAIEN << 13)
    | (PGAQEN << 12)
    | (STRMEN << 11)
    | (STRMSTART << 10)
    | (STRMSTOP << 9)
    | (STRMCOUNT << 6)
    | (STRMBITS << 4)
    | (STAMPEN << 3)
    | (TIMESYNCEN << 2)
    | (DATASYNCEN << 1)
    | (SSTRMRST << 0);
    }
};

struct PLLConfiguration {
    uint32_t VCOEN = 1;
    uint32_t IVCO = 0;
    uint32_t REFOUTEN = 1;
    uint32_t REFDIV = 0b11;
    uint32_t IXTAL = 0b01;
    uint32_t XTALCAP = 0b10000;
    uint32_t LDMUX = 0b0000;
    uint32_t ICP = 0;
    uint32_t PFDEN = 0;
    uint32_t CPTEST = 0b000;
    uint32_t INT_PLL = 1;
    uint32_t PWRSAV = 0;

    uint32_t encode() {
        return (VCOEN << 27)
        | (IVCO << 26)
        | (REFOUTEN << 24)
        | (1 << 23)
        | (REFDIV << 21)
        | (IXTAL << 19)
        | (XTALCAP << 14)
        | (LDMUX << 10)
        | (ICP << 9)
        | (PFDEN << 8)
        | (CPTEST << 4)
        | (INT_PLL << 3);
    }
};

struct PLLIntegerDivisionRatio {
    uint32_t NDIV = 1536;
    uint32_t RDIV = 16;

    uint32_t encode() {
        return (NDIV << 13) | (RDIV << 3);
    }
};

struct PLLDivisionRatio {
    uint32_t FDIV = 0x80000;

    uint32_t encode() {
        return (FDIV << 8) | (0b01110000);
    }
};

struct DSPInterface {
    uint32_t FRAMECOUNT = 0x8000000;

    uint32_t encode() {
        return FRAMECOUNT;
    }
};


struct ClockFractionalDivisionRatio {
    uint32_t L_CNT = 256;
    uint32_t M_CNT = 1563;
    uint32_t FCLKIN = 0;
    uint32_t ADCCLK = 0;
    uint32_t SERCLK = 1;
    uint32_t MODE = 0;

    uint32_t encode() {
        return (L_CNT << 16)
        | (M_CNT << 4)
        | (FCLKIN << 3)
        | (ADCCLK << 2)
        | (SERCLK << 1)
        | (MODE << 0);
    }
};

struct TestMode1 {
    uint32_t encode() {
        return 0x1E0F401;
    }
};

struct TestMode2 {
    uint32_t encode() {
        return 0x14C0402;
    }
};


void setup() {
    pinMode(CS, OUTPUT);
    SPI.begin(SCK, 37, MOSI);
    digitalWrite(CS, HIGH);
    Configuration1 conf1 = Configuration1();
    Configuration2 conf2 = Configuration2();
    Configuration3 conf3 = Configuration3();

    conf2.IQEN = 0;
    conf2.FORMAT = 0b10;
    conf2.BITS = 0b100;
    conf3.STRMCOUNT = 0b000;
    conf3.STRMEN = 0b1;
    conf3.STAMPEN = 0b1;
    conf3.TIMESYNCEN = 0b0;
    conf3.DATASYNCEN = 0b0;

    write_reg(MaxRegister::CONF1,   conf1.encode());
    write_reg(MaxRegister::CONF2,   conf2.encode());
    write_reg(MaxRegister::CONF3,   conf3.encode());
    write_reg(MaxRegister::PLLCONF, PLLConfiguration().encode());
    write_reg(MaxRegister::DIV,     PLLIntegerDivisionRatio().encode());
    write_reg(MaxRegister::FDIV,    PLLDivisionRatio().encode());
    write_reg(MaxRegister::STRM,    DSPInterface().encode());
    write_reg(MaxRegister::CLK,     ClockFractionalDivisionRatio().encode());
    write_reg(MaxRegister::TEST1,   TestMode1().encode());
    write_reg(MaxRegister::TEST2,   TestMode2().encode());
    
    while(true) {
        // conf3.STRMSTART = 0;
        // write_reg(MaxRegister::CONF3, conf3.encode());
        // delay(500);
        conf3.STRMSTART = 1;
        write_reg(MaxRegister::CONF3, conf3.encode());
        delay(500);
        // conf3.STRMSTOP = 1;
        // write_reg(MaxRegister::CONF3, conf3.encode());
        // delay(500);
        // conf3.STRMSTOP = 0;
        // write_reg(MaxRegister::CONF3, conf3.encode());
        // delay(500);
    }
    
}

void loop() {

}