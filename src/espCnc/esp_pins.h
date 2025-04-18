#pragma once
#include<cstdint>

#if BOARD_REV == 0

namespace Pins {
    constexpr uint8_t BOOT_SW = 0;
    constexpr uint8_t EXT0 = 3;
    constexpr uint8_t EXT1 = 4;
    constexpr uint8_t DIAG_A = 5;
    constexpr uint8_t INDEX_A = 6;
    constexpr uint8_t SDA_A = 7;
    constexpr uint8_t STEP_A = 8;
    constexpr uint8_t DIR_A = 9;
    constexpr uint8_t DIAG_B = 10;
    constexpr uint8_t INDEX_B = 11;
    constexpr uint8_t SDA_B = 12;
    constexpr uint8_t LED_0 = 13;
    constexpr uint8_t LED_1 = 14;
    constexpr uint8_t LED_2 = 15;
    constexpr uint8_t LED_3 = 16;
    constexpr uint8_t STEP_B = 17;
    constexpr uint8_t DIR_B = 18;
    constexpr uint8_t USB_DN = 19;
    constexpr uint8_t USB_DP = 20;
    constexpr uint8_t IO_INT = 21;
    constexpr uint8_t IO_RESET = 26;
    constexpr uint8_t STEP_D = 33;
    constexpr uint8_t SDA_D = 34;
    constexpr uint8_t INDEX_D = 35;
    constexpr uint8_t DIAG_D = 36;
    constexpr uint8_t LDAC = 37;
    constexpr uint8_t SDA = 38;
    constexpr uint8_t SCL = 39;
    constexpr uint8_t DIR_C = 40;
    constexpr uint8_t STEP_C = 41;
    constexpr uint8_t SDA_C = 42;
    constexpr uint8_t UART_TX = 43;
    constexpr uint8_t UART_RX = 44;
    constexpr uint8_t INDEX_C = 45;
    constexpr uint8_t DIAG_C = 46;
    constexpr uint8_t DIR_D = 47;
    constexpr uint8_t BUZZER = 48;
    constexpr uint8_t MS2_B = 0;
    constexpr uint8_t MS1_B = 1;
    constexpr uint8_t SPREAD_B = 2;
    constexpr uint8_t ENN_B = 3;
    constexpr uint8_t MS2_A = 4;
    constexpr uint8_t MS1_A = 5;
    constexpr uint8_t SPREAD_A = 6;
    constexpr uint8_t ENN_A = 7;
    constexpr uint8_t MS2_D = 8;
    constexpr uint8_t MS1_D = 9;
    constexpr uint8_t SPREAD_D = 10;
    constexpr uint8_t ENN_D = 11;
    constexpr uint8_t MS2_C = 12;
    constexpr uint8_t MS1_C = 13;
    constexpr uint8_t SPREAD_C = 14;
    constexpr uint8_t ENN_C = 15;
}

#endif
#if BOARD_REV==1

namespace Pins {
    constexpr uint8_t BOOT_SW = 0;
    constexpr uint8_t DIR_0 = 1;
    constexpr uint8_t STEP_0 = 2;
    constexpr uint8_t SDA_0 = 3;
    constexpr uint8_t ENN_0 = 4;
    constexpr uint8_t DIR_3 = 5;
    constexpr uint8_t SDA_3 = 6;
    constexpr uint8_t STEP_3 = 7;
    constexpr uint8_t ENN_3 = 8;
    constexpr uint8_t SCL = 9;
    constexpr uint8_t DIR_1 = 10;
    constexpr uint8_t STEP_1 = 11;
    constexpr uint8_t SDA_1 = 12;
    constexpr uint8_t ENN_1 = 13;
    constexpr uint8_t DIR_2 = 14;
    constexpr uint8_t STEP_2 = 15;
    constexpr uint8_t SDA_2 = 16;
    constexpr uint8_t ENN_2 = 17;
    constexpr uint8_t USB_DN = 19;
    constexpr uint8_t USB_DP = 20;
    constexpr uint8_t SDA = 21;
    constexpr uint8_t TP22 = 26;
    constexpr uint8_t TP23 = 33;
    constexpr uint8_t TP24 = 34;
    constexpr uint8_t TP25 = 35;
    constexpr uint8_t TP26 = 36;
    constexpr uint8_t TP27 = 37;
    constexpr uint8_t TP28 = 38;
    constexpr uint8_t TP29 = 39;
    constexpr uint8_t LED_3 = 40;
    constexpr uint8_t LED_2 = 41;
    constexpr uint8_t LED_1 = 42;
    constexpr uint8_t LED_0 = 45;
}
#endif