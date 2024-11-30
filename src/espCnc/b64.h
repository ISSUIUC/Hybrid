#pragma once

#include<cstdint>
#include<stddef.h>

void base64_encode(uint8_t const* buf, unsigned int bufLen, uint8_t* out, size_t* out_len);