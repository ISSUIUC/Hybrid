#pragma once

typedef void (*CommandDataCallback) (uint8_t*, size_t);    

void setup_wifi(const char * ssid, const char * pass);
void set_data_callback(CommandDataCallback callback);
