#ifndef TMCDRIVER_H_
#define TMCDRIVER_H_

#include <stdint.h>

class TMC2660 
{
    public:
        TMC2660(uint8_t cs_pin, uint8_t sg_pin)
        {
            cs = cs_pin;
            sg = sg_pin;
        }
        
        void init();
        void write(uint32_t*);
        void pushCommands(void);

        //DRVCONF
        void slopeControlHigh(uint8_t); //SLPH Controls the current slope for turning on the MOSFET (MAX this out) 
        void slopeControlLow(uint8_t); //SLPL Controls the current slope for turning off the MOSFET (MAX this out also)
        void enableDetectGND(bool);  //DISS2G    0: Short to GND protection is enabled      1: Disabled
        void motorShortTimer(uint8_t);    //TS2G short to ground timer. (turns off power mosfets)
        void stepMode(bool);   //SDOFF 0: step and dir interface, 1: SPI interface 
        void senseResScale(bool);  //VSENSE
        void readMode(uint8_t); //RDSEL Microstep positon or stallguard or coolStep current level readback

        //SGCSCONF
        void filterMode(bool); //SFILT 0: standard mode, rapid stall detection 1: Filtered mode for precise load measurement.
        void stallGrdThresh(int8_t); //SGT the higher the value, the higher the torque required to indicate a stall, start with 0.
        void currentScale(uint8_t); //CS

        //SMARTEN
        void minCoilCurrent(bool); //SEIMIN 0: 1/2 of CS    1: 1/4 of CS
        void coilDecrementSpd(uint8_t); //SEDN number of times stallguard2 is above threshold is sampled before decrement.
        void coilUpperThreshold(uint8_t); //SEMAX Threshold (high) is (SEMIN + SEMAX + 1)*32 decreases current scaling when above threshold.
        void coilLowerThreshold(uint8_t); //SEMIN Threshold (low) is (SEMIN)*32 increases current scaling when below threshold.
        void coilIncrementSize(uint8_t); //SEUP The magnitude of compensation when the coil current drops below the low threshold.

        //CHOPCONF
        void blankTime(uint8_t); //TBL the number of cycles after a switch, in which the sense resistor ringing is ignored (ripples are bad).
        void chopperMode(bool); //CHM 1: constant off-time mode    0: spreadCycle mode
        //Spreadcycle mode (dynamic TOFF mode)
        void hystEnd(int8_t); //HEND sets minimum allowable offset value from target current.
        void hystStart(uint8_t); //HSTRT sets maximum allowable offset value from target current.
        void hystDecrement(uint8_t); //HDEC sets the rate at which the value decrements from HSTRT+HEND to HEND.
        //Constant TOFF mode
        void slowDecayTime(uint8_t); //TOFF   t = (1/fclk)*((TOFF * 32) + 12)

        //DRVCTRL_STEP/DIR_MODE
        void setMicroStep(uint16_t); //MRES
        void doubleStepping(bool); //DEDGE 0: Rising step pulse is active, falling is inactive    1: Both rising and falling edge active
        void stepInterpolation(bool); //INTPOL 0: Disable STEP pulse interpolation   1: Enable step pulse multiplication by 16.

        //TODO: DRVCTRL_SPI_MODE


    private:
        uint32_t DRVCTRL_0_CMD = 0b00 << 18;
        uint32_t DRVCTRL_1_CMD;
        uint32_t DRVCONF_CMD = 0b111 << 17;
        uint32_t CHOPCONF_CMD = 0b100 << 17;
        uint32_t SMARTEN_CMD = 0b101 << 17;
        uint32_t SGCSCONF_CMD = 0b110 << 17;

        //Hardawre pins
        uint8_t cs;
        uint8_t sg;

        //SPI Speed 
        const uint32_t spi_speed = 16000000/8; // Default 2MHz

        //Bitfield construction
        void modifyBits(uint32_t mask, uint32_t bits, uint32_t* reg);

};



#endif