/*
 * WebSocketTest.ino
 *
 * Minimal WebSocket test to isolate connection issues.
 * This sketch only tests WebSocket functionality without servo communication.
 *
 * Hardware Requirements:
 * - ESP32 board
 *
 * Written by Nicholas Stedman (nick@robotstack.com)
 */

#include <WiFi.h>
#include <WebSocketsServer.h>

#define IO Serial

// WiFi credentials
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// Create WebSocket server
WebSocketsServer webSocket = WebSocketsServer(8080);

// Connection tracking
bool wifiConnected = false;
bool wsServerStarted = false;
unsigned long lastStatusPrint = 0;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    IO.print("WebSocket event: ");
    IO.print(type);
    IO.print(" (client ");
    IO.print(num);
    IO.println(")");

    switch (type)
    {
    case WStype_DISCONNECTED:
        IO.printf("[%u] Disconnected!\n", num);
        break;

    case WStype_CONNECTED:
    {
        IPAddress ip = webSocket.remoteIP(num);
        IO.printf("[%u] Connected from %d.%d.%d.%d\n",
                  num, ip[0], ip[1], ip[2], ip[3]);
    }
    break;

    case WStype_TEXT:
        IO.printf("[%u] Received text: %s\n", num, payload);
        // Echo back the message
        webSocket.sendTXT(num, payload);
        break;

    case WStype_BIN:
        IO.printf("[%u] Received binary length: %u\n", num, length);
        // Echo back the binary data
        webSocket.sendBIN(num, payload, length);
        break;

    case WStype_ERROR:
        IO.printf("[%u] Error!\n", num);
        break;

    case WStype_FRAGMENT_TEXT_START:
        IO.printf("[%u] Fragment text start\n", num);
        break;

    case WStype_FRAGMENT_BIN_START:
        IO.printf("[%u] Fragment bin start\n", num);
        break;

    case WStype_FRAGMENT:
        IO.printf("[%u] Fragment\n", num);
        break;

    case WStype_FRAGMENT_FIN:
        IO.printf("[%u] Fragment fin\n", num);
        break;

    default:
        IO.printf("[%u] Unknown event type: %u\n", num, type);
        break;
    }
}

void wifiEvent(WiFiEvent_t event)
{
    IO.print("WiFi event: ");
    IO.println(event);

    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_START:
        IO.println("WiFi client started");
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        IO.println("WiFi client stopped");
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        IO.println("Connected to WiFi");
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        IO.println("Disconnected from WiFi");
        wifiConnected = false;
        wsServerStarted = false;
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        IO.print("Got IP: ");
        IO.println(WiFi.localIP());
        wifiConnected = true;
        break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        IO.println("Lost IP address");
        wifiConnected = false;
        wsServerStarted = false;
        break;
    }
}

void setup()
{
    // Initialize Serial
    IO.begin(1000000);
    delay(1000);

    IO.println("Starting WebSocket Test...");

    // Configure WiFi
    WiFi.onEvent(wifiEvent);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);

    // Begin connection
    IO.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    // Wait for connection
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 20000)
    {
        delay(500);
        IO.print(".");
    }
    IO.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        IO.println("WiFi connected!");
        IO.print("IP: ");
        IO.println(WiFi.localIP());
        IO.print("RSSI: ");
        IO.println(WiFi.RSSI());

        // Start WebSocket server
        webSocket.begin();
        webSocket.onEvent(webSocketEvent);
        wsServerStarted = true;

        IO.println("WebSocket server started on port 80");
        IO.print("Connect to: ws://");
        IO.print(WiFi.localIP());
        IO.println(":80");
    }
    else
    {
        IO.println("WiFi connection failed!");
        IO.print("Status: ");
        IO.println(WiFi.status());
    }
}

void loop()
{
    // Handle WebSocket events
    if (wsServerStarted)
    {
        webSocket.loop();
    }

    // Print status every 5 seconds
    if (millis() - lastStatusPrint >= 5000)
    {
        lastStatusPrint = millis();

        if (wifiConnected)
        {
            IO.print("Status: WiFi OK | IP: ");
            IO.print(WiFi.localIP());
            IO.print(" | RSSI: ");
            IO.print(WiFi.RSSI());
            IO.print(" dBm | WebSocket: ");
            IO.println(wsServerStarted ? "Running" : "Stopped");
        }
        else
        {
            IO.print("Status: WiFi disconnected | Status: ");
            IO.println(WiFi.status());
        }
    }

    // Check for connection loss
    if (wifiConnected && WiFi.status() != WL_CONNECTED)
    {
        IO.println("WiFi connection lost!");
        wifiConnected = false;
        wsServerStarted = false;
    }

    delay(100);
}