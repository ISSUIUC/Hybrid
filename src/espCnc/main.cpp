#include<Arduino.h>
// #include<TMCStepper.h>
#include<TMCDriver.h>
#include<SPI.h>

#define SPI_SCK 3
#define SPI_MOSI 2
#define SPI_MISO 1
#define EN_0 12
#define CS_0 6
#define STEP_0 4
#define DIR_0 5
#define SG_0 7
#define EN_1 42
#define CS_1 41
#define STEP_1 43
#define DIR_1 44
#define SG_1 45
#define LED0 9
#define LED1 8

TMC2660 driver(CS_0, SG_0);

bool sdoff = false;

void setup() {
    Serial.begin(9600);
    SPI.begin(SPI_SCK,SPI_MISO,SPI_MOSI);
    pinMode(LED0, OUTPUT);
    pinMode(LED1, OUTPUT);
    pinMode(EN_0, OUTPUT);
    pinMode(EN_1, OUTPUT);
    pinMode(STEP_0, OUTPUT);
    pinMode(STEP_1, OUTPUT);
    pinMode(DIR_0, OUTPUT);
    pinMode(DIR_1, OUTPUT);
    pinMode(CS_0, OUTPUT);
    pinMode(CS_1, OUTPUT);

driver.init();
//CHOPCONF settings
driver.blankTime(24);
driver.chopperMode(1);
driver.hystEnd(5);
driver.hystStart(6);
driver.hystDecrement(32);
driver.slowDecayTime(5);

// DRVCONF settings
driver.readMode(1);
driver.senseResScale(0);
driver.stepMode(0);
driver.motorShortTimer(2);
driver.enableDetectGND(0);
driver.slopeControlLow(2);
driver.slopeControlHigh(2);

//SMARTEN settings
driver.coilLowerThreshold(8);
driver.coilIncrementSize(8);
driver.coilUpperThreshold(8);
driver.coilDecrementSpd(8);
driver.minCoilCurrent(0);

//SGCSCONF settings
driver.currentScale(25);
driver.stallGrdThresh(0);
driver.filterMode(0);
driver.setMicroStep(32);

//Write the constructed bitfields to the driver
driver.pushCommands();
}

void step(int channel, bool dir) {
    if(channel == 0) {
        digitalWrite(DIR_0, dir);
        digitalWrite(STEP_0, HIGH);
        delayMicroseconds(1);
        digitalWrite(STEP_0, LOW);
    }
    if(channel == 1 ) {
        digitalWrite(DIR_1, dir);
        digitalWrite(STEP_1, HIGH);
        delayMicroseconds(1);
        digitalWrite(STEP_1, LOW);
    }   
}

bool dir = false;
int d = 1000;
void loop() {
    for(int i = 0; i < 2000; i += 1) {
        step(0, dir);
        step(1, dir);
        delayMicroseconds(d);
    }
    d *= 0.95;
    dir = !dir;    
}