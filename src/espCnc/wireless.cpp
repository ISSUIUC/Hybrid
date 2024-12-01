#include"wireless.h"

#ifdef DOWIFI
#include"b64.h"

#include "web/index_string.h"

void WirelessServer::handle_ws_message(void *arg, uint8_t *data, size_t len, size_t client_id) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;

    if(!info->final) {
        Serial.println("Err non final msg");
        return;
    }

    if(info->len > cmd_buffer_size) {
        Serial.println("Err too large msg");
        return;
    }

    if(info->len <= cmd_buffer_size) {
        memcpy(cmd_buffer + info->index, data, len);
    }

    if(info->final && info->index + len == info->len) {
        finish_ws_message(info->len);
    }
}

void WirelessServer::finish_ws_message(size_t len) {
    if(callback) callback(cmd_buffer, len);
}


void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len, WirelessServer* parent_server) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            parent_server->handle_ws_message(arg, data, len, client->id());
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void WirelessServer::periodic_notify() {
    ws.cleanupClients();
    static_assert(sizeof(Status) % 3 == 0);
    uint8_t status_buff[sizeof(Status) * 4 / 3+1]{};
    Status to_send = status;
    size_t out_len = sizeof(status_buff);
    base64_encode((uint8_t*)&status, sizeof(to_send), status_buff, &out_len);
    ws.textAll((const char*)status_buff);
}

void periodicManager(void * arg){
    WirelessServer* server = (WirelessServer*)arg;
    while(true) {
        server->periodic_notify();
        delay(1000);
    }
}

bool WirelessServer::setup_wifi(const char * ssid, const char * password) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    WiFi.setHostname("espcnc");
    WiFi.config(INADDR_NONE,INADDR_NONE,INADDR_NONE,INADDR_NONE);
    WiFi.begin(ssid, password);
    Serial.println("connecting...");
    while(WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
    }

    Serial.println(WiFi.localIP());
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req){
        req->send_P(200, "text/html", index_string);
    });
    server.on("/bundle.js", HTTP_GET, [](AsyncWebServerRequest* req){
        req->send_P(200, "text/javascript", js_string);
    });
    ws.onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void * arg, uint8_t * data, size_t len){
        onEvent(server, client, type, arg, data, len, this);
    });
    server.addHandler(&ws);
    server.begin();
    xTaskCreatePinnedToCore(periodicManager, "periodics", 8192*2, this, 1, nullptr, 1);
    return true;
}
#endif