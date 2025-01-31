#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "wireless.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

CommandDataCallback cmd_data_callback;

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    cmd_data_callback(data,len);
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
    }
}

void set_data_callback(CommandDataCallback callback) {
    cmd_data_callback = callback;
}

void setup_wifi(const char *ssid, const char *pass)
{
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Connecting");
        delay(1000);
    }
    Serial.println(WiFi.localIP());
    ws.onEvent(onEvent);
    server.addHandler(&ws);
    server.begin();
}
