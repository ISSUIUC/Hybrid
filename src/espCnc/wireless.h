#pragma once

#ifdef DOWIFI

#include<wifi.h>
#include<ESPAsyncWebServer.h>
#include<cstdint>

#include "status.h"
#include "queue.h"

class WirelessServer {
public:
    WirelessServer(uint8_t* cmd_buffer, size_t len): 
        cmd_buffer(cmd_buffer), cmd_buffer_size(len), server(80), ws("/ws"){
        }
    bool setup_wifi(const char * ssid, const char * password);
    void on_message(std::function<void(uint8_t*,size_t)> callback_fn) {
        callback = callback_fn;
    }
    void set_status(Status status) {
        this->status = status;
    }

private:
    friend void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len, WirelessServer* parent_server);
    friend void periodicManager(void * arg);
    void handle_ws_message(void *arg, uint8_t *data, size_t len, size_t client_id);
    void finish_ws_message(size_t len);
    void periodic_notify();
    uint8_t* cmd_buffer = nullptr;
    size_t cmd_buffer_size = 0;
    AsyncWebServer server;
    AsyncWebSocket ws;
    std::function<void(uint8_t*,size_t)> callback;
    Status status{};
};

#else

#include<functional>
#include"status.h"

class WirelessServer {
public:
    WirelessServer(uint8_t* cmd_buffer, size_t len) {}
    bool setup_wifi(const char * ssid, const char * password) { return true; }
    void on_message(std::function<void(uint8_t*,size_t)> callback_fn) {}
    void set_status(Status status) {}
};

#endif