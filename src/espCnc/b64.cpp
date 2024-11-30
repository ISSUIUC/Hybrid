#include "b64.h"

#include<cstring>

static constexpr uint8_t base64_chars[] =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

void base64_encode(uint8_t const* buf, unsigned int bufLen, uint8_t* out, size_t* out_len) {
    if(!out_len) return;
    if(*out_len < bufLen * 4 / 3) return;
    if(bufLen % 3 != 0) return;

    int i = 0;
    uint8_t char_array_3[3]{};
    uint8_t char_array_4[4]{};

    while (bufLen--) {
        char_array_3[i++] = *(buf++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(int i = 0; i < 4; i++){
                *out++ = base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    *out_len = bufLen * 4 / 3;
}