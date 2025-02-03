#include<Arduino.h>
#include<SPI.h>
#include"pins.h"

SPISettings spiSettings = SPISettings(2000000, MSBFIRST, SPI_MODE0);

#define XTAL_FREQ (double)32000000
#define FREQ_DIV (double)pow(2.0, 25.0)
#define FREQ_STEP (double)(XTAL_FREQ / FREQ_DIV)

/*!
 * \brief Represents the interruption masks available for the radio
 *
 * \remark Note that not all these interruptions are available for all packet types
 */
typedef enum
{
	IRQ_RADIO_NONE = 0x0000,
	IRQ_TX_DONE = 0x0001,
	IRQ_RX_DONE = 0x0002,
	IRQ_PREAMBLE_DETECTED = 0x0004,
	IRQ_SYNCWORD_VALID = 0x0008,
	IRQ_HEADER_VALID = 0x0010,
	IRQ_HEADER_ERROR = 0x0020,
	IRQ_CRC_ERROR = 0x0040,
	IRQ_CAD_DONE = 0x0080,
	IRQ_CAD_ACTIVITY_DETECTED = 0x0100,
	IRQ_RX_TX_TIMEOUT = 0x0200,
	IRQ_RADIO_ALL = 0xFFFF,
} RadioIrqMasks_t;


typedef enum RadioCommands_e
{
	RADIO_GET_STATUS = 0xC0,
	RADIO_WRITE_REGISTER = 0x0D,
	RADIO_READ_REGISTER = 0x1D,
	RADIO_WRITE_BUFFER = 0x0E,
	RADIO_READ_BUFFER = 0x1E,
	RADIO_SET_SLEEP = 0x84,
	RADIO_SET_STANDBY = 0x80,
	RADIO_SET_FS = 0xC1,
	RADIO_SET_TX = 0x83,
	RADIO_SET_RX = 0x82,
	RADIO_SET_RXDUTYCYCLE = 0x94,
	RADIO_SET_CAD = 0xC5,
	RADIO_SET_TXCONTINUOUSWAVE = 0xD1,
	RADIO_SET_TXCONTINUOUSPREAMBLE = 0xD2,
	RADIO_SET_PACKETTYPE = 0x8A,
	RADIO_GET_PACKETTYPE = 0x11,
	RADIO_SET_RFFREQUENCY = 0x86,
	RADIO_SET_TXPARAMS = 0x8E,
	RADIO_SET_PACONFIG = 0x95,
	RADIO_SET_CADPARAMS = 0x88,
	RADIO_SET_BUFFERBASEADDRESS = 0x8F,
	RADIO_SET_MODULATIONPARAMS = 0x8B,
	RADIO_SET_PACKETPARAMS = 0x8C,
	RADIO_GET_RXBUFFERSTATUS = 0x13,
	RADIO_GET_PACKETSTATUS = 0x14,
	RADIO_GET_RSSIINST = 0x15,
	RADIO_GET_STATS = 0x10,
	RADIO_RESET_STATS = 0x00,
	RADIO_CFG_DIOIRQ = 0x08,
	RADIO_GET_IRQSTATUS = 0x12,
	RADIO_CLR_IRQSTATUS = 0x02,
	RADIO_CALIBRATE = 0x89,
	RADIO_CALIBRATEIMAGE = 0x98,
	RADIO_SET_REGULATORMODE = 0x96,
	RADIO_GET_ERROR = 0x17,
	RADIO_CLR_ERROR = 0x07,
	RADIO_SET_TCXOMODE = 0x97,
	RADIO_SET_TXFALLBACKMODE = 0x93,
	RADIO_SET_RFSWITCHMODE = 0x9D,
	RADIO_SET_STOPRXTIMERONPREAMBLE = 0x9F,
	RADIO_SET_LORASYMBTIMEOUT = 0xA0,
} RadioCommands_t;

/*!
 * \brief Represents the operating mode the radio is actually running
 */
typedef enum
{
	MODE_SLEEP = 0x00, //! The radio is in sleep mode
	MODE_STDBY_RC,	   //! The radio is in standby mode with RC oscillator
	MODE_STDBY_XOSC,   //! The radio is in standby mode with XOSC oscillator
	MODE_FS,		   //! The radio is in frequency synthesis mode
	MODE_TX,		   //! The radio is in transmit mode
	MODE_RX,		   //! The radio is in receive mode
	MODE_RX_DC,		   //! The radio is in receive duty cycle mode
	MODE_CAD		   //! The radio is in channel activity detection mode
} RadioOperatingModes_t;

void E22WaitOnBusy(void)
{
	int timeout = 1000;
	while (digitalRead(E22_BUSY) == HIGH)
	{
		delay(1);
		timeout -= 1;
		if (timeout < 0)
		{
			Serial.println("ERR Wait too long on busy");
			return;
		}
	}
}

void E22WriteCommand(RadioCommands_t command, uint8_t* buffer, size_t size) {
    E22WaitOnBusy();

    digitalWrite(E22_CS, LOW);
    SPI.beginTransaction(spiSettings);
    SPI.transfer((uint8_t)command);
    for(size_t i = 0; i < size; i++){
        SPI.transfer(buffer[i]);
    }
    SPI.endTransaction();
    digitalWrite(E22_CS, HIGH);

    E22WaitOnBusy();
}

void E22WriteCommandSingle(RadioCommands_t command, uint8_t param) {
    E22WriteCommand(command, &param, 1);
}

void E22ReadCommand(RadioCommands_t command, uint8_t* buffer, size_t size) {
    E22WaitOnBusy();

    digitalWrite(E22_CS, LOW);
    SPI.beginTransaction(spiSettings);
    SPI.transfer((uint8_t)command);
    SPI.transfer(0x00);
    for(size_t i = 0; i < size; i++){
        buffer[0] = SPI.transfer(0x00);
    }
    SPI.endTransaction();
    digitalWrite(E22_CS, HIGH);

    E22WaitOnBusy();
}

void E22WriteBuffer(uint8_t offest, uint8_t* buffer, size_t size) {
    E22WaitOnBusy();

    digitalWrite(E22_CS, LOW);
    SPI.beginTransaction(spiSettings);
    SPI.transfer(RADIO_WRITE_BUFFER);
    SPI.transfer(offest);
    for(size_t i = 0; i < size; i++) {
        SPI.transfer(buffer[i]);
    }
    SPI.endTransaction();
    digitalWrite(E22_CS, HIGH);

    E22WaitOnBusy();
}

void E22WriteRegisters(uint16_t address, uint8_t* buffer, size_t size) {
    E22WaitOnBusy();

    digitalWrite(E22_CS, LOW);
    SPI.beginTransaction(spiSettings);
    SPI.transfer(RADIO_WRITE_REGISTER);
    SPI.transfer((address & 0xFF00) >> 8);
    SPI.transfer(address & 0x00FF);

    for(size_t i = 0; i < size; i++){
        SPI.transfer(buffer[i]);
    }

    SPI.endTransaction();
    digitalWrite(E22_CS, HIGH);

    E22WaitOnBusy();
}

void E22ReadRegisters(uint16_t address, uint8_t* buffer, size_t size) {
    E22WaitOnBusy();

    digitalWrite(E22_CS, LOW);
    SPI.beginTransaction(spiSettings);
    SPI.transfer32(RADIO_READ_REGISTER);
    SPI.transfer((address & 0xFF00) >> 8);
    SPI.transfer(address & 0x00FF);
    SPI.transfer(0x00);
    for(size_t i = 0; i < size; i++){
        buffer[i] = SPI.transfer(0x00);
    }
    SPI.endTransaction();
    digitalWrite(E22_CS, HIGH);

    E22WaitOnBusy();
}


void E22CalibrateImage(uint32_t freq)
{
	uint8_t calFreq[2];

	if (freq > 900000000)
	{
		calFreq[0] = 0xE1;
		calFreq[1] = 0xE9;
	}
	else if (freq > 850000000)
	{
		calFreq[0] = 0xD7;
		calFreq[1] = 0xDB;
	}
	else if (freq > 770000000)
	{
		calFreq[0] = 0xC1;
		calFreq[1] = 0xC5;
	}
	else if (freq > 460000000)
	{
		calFreq[0] = 0x75;
		calFreq[1] = 0x81;
	}
	else if (freq > 425000000)
	{
		calFreq[0] = 0x6B;
		calFreq[1] = 0x6F;
	}
	E22WriteCommand(RADIO_CALIBRATEIMAGE, calFreq, 2);
}

void E22SetFrequency(uint32_t frequency) {
    uint8_t buf[4];
	uint32_t freq = 0;

    E22CalibrateImage(frequency);
	freq = (uint32_t)((double)frequency / (double)FREQ_STEP);
	buf[0] = (uint8_t)((freq >> 24) & 0xFF);
	buf[1] = (uint8_t)((freq >> 16) & 0xFF);
	buf[2] = (uint8_t)((freq >> 8) & 0xFF);
	buf[3] = (uint8_t)(freq & 0xFF);
	E22WriteCommand(RADIO_SET_RFFREQUENCY, buf, 4);
}

void E22_setup(){
    pinMode(E22_CS, OUTPUT);
    pinMode(E22_DI01, INPUT);
    pinMode(E22_DI03, INPUT);
    pinMode(E22_BUSY, INPUT);
    pinMode(E22_RXEN, OUTPUT);
    pinMode(E22_RESET, OUTPUT);
    digitalWrite(E22_CS, HIGH);
    digitalWrite(E22_RESET, LOW);
    delay(10);
    digitalWrite(E22_RESET, HIGH);
    E22WaitOnBusy();
}

void E22SetDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask)
{
	uint8_t buf[8];

	buf[0] = (uint8_t)((irqMask >> 8) & 0x00FF);
	buf[1] = (uint8_t)(irqMask & 0x00FF);
	buf[2] = (uint8_t)((dio1Mask >> 8) & 0x00FF);
	buf[3] = (uint8_t)(dio1Mask & 0x00FF);
	buf[4] = (uint8_t)((dio2Mask >> 8) & 0x00FF);
	buf[5] = (uint8_t)(dio2Mask & 0x00FF);
	buf[6] = (uint8_t)((dio3Mask >> 8) & 0x00FF);
	buf[7] = (uint8_t)(dio3Mask & 0x00FF);
	E22WriteCommand(RADIO_CFG_DIOIRQ, buf, 8);
}

void check() {
    int io3 = digitalRead(E22_DI03);
    int io1 = digitalRead(E22_DI01);
    int busy = digitalRead(E22_BUSY);

    Serial.print(io3);
    Serial.print(',');
    Serial.print(io1);
    Serial.print(',');
    Serial.print(busy);
    Serial.print(',');
    Serial.println();
}

void transmit(){
    static int loops = 0;
    loops++;
    uint8_t STANDBY_RC = 0;
    uint8_t LORA_MODE = 1;
    uint32_t frequency = 430000000;
    E22WriteCommandSingle(RADIO_SET_STANDBY, STANDBY_RC);
    E22WriteCommandSingle(RADIO_SET_PACKETTYPE, LORA_MODE);
    E22SetFrequency(frequency);
    uint8_t txparams[] = {/*dbm*/ 0x00, /*ramp 800us*/ 0x05};
    E22WriteCommand(RADIO_SET_TXPARAMS, txparams, sizeof(txparams));
    uint8_t baseaddr[] = {/*tx*/0x00, /*rx*/0x00};
    E22WriteCommand(RADIO_SET_BUFFERBASEADDRESS, baseaddr, sizeof(baseaddr));
    uint8_t packet[] = {loops, 0, 0, 0};
    E22WriteBuffer(0x00, packet, sizeof(packet));
    uint8_t modulation_params[] = {
        0x08, //Spreading Factor 8
        0x05, //Bandwidth 250k
        0x04, //CR 4/8
        0x00, //Low data rate off
    };
    E22WriteCommand(RADIO_SET_MODULATIONPARAMS, modulation_params, sizeof(modulation_params));
    uint8_t packet_params[] = {
        0x00, //Hi bit preamble
        0x0a, //Lo bit preamble
        0x00, //Explicit header
        0x04, //Payload len
        0x00, //No crc
        0x00, //Standard iq
    };
    E22WriteCommand(RADIO_SET_PACKETPARAMS, packet_params, sizeof(packet_params));
    E22SetDioIrqParams(
        IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
        IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT,
        IRQ_RADIO_NONE,
        IRQ_RADIO_NONE);

    uint8_t timeout[] = {
        0x00,0x00,0x00 //no timeout
    };
    E22WriteCommand(RADIO_SET_TX, timeout, sizeof(timeout));
    for(int i = 0; i < 1000; i++){
    }
    uint8_t clear_irq[] = {
        0xff,0xff //clear all
    };
    E22WriteCommand(RADIO_CLR_IRQSTATUS, clear_irq, sizeof(clear_irq));
    Serial.println("TX");
    delay(1000);
}   