#ifndef BITFIELDS_H_
#define BITFIELDS_H_

struct DRVCONF
{
    const uint32_t rdsel = 0b11 << 4;
    const uint32_t vsense = 0b1 << 6;
    const uint32_t sdoff = 0b1 << 7;
    const uint32_t ts2g = 0b11 << 8;
    const uint32_t diss2g = 0b1 << 10; 
    const uint32_t slpl = 0b11 << 12;
    const uint32_t slph = 0b11 << 14;
    const uint32_t tst = 0b1 << 16;
};

struct DRVCTRL_0       //STEP and DIR mode
{
    const uint32_t mstep = 0b1111;
    const uint32_t dedge = 0b1 << 8;
    const uint32_t intpol = 0b1 << 9;
};

struct DRVCTRL_1       //SPI mode
{
    const uint32_t address = 0x00;

};

struct CHOPCONF
{
    const uint32_t toff = 0b1111;
    const uint32_t hstrt = 0b111 << 4;
    const uint32_t hend = 0b1111 << 7;
    const uint32_t hdec = 0b11 << 11;
    const uint32_t rndtf = 0b1 << 13;
    const uint32_t chm = 0b1 << 14;
    const uint32_t tbl = 0b11 << 15;
};

struct SMARTEN
{
    const uint32_t semin = 0b1111;
    const uint32_t seup = 0b11 << 5;
    const uint32_t semax = 0b1111 << 8;
    const uint32_t sedn = 0b11 << 13;
    const uint32_t seimin = 0b1 << 15;
};

struct SGCSCONF
{
    const uint32_t csc = 0b11111;
    const uint32_t sgt = 0b1111111 << 8;
    const uint32_t sfilt = 0b1 << 16; 
};


#endif
