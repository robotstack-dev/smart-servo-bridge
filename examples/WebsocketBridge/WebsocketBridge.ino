/*
 * WebSocketBridge.ino
 *
 * Example sketch for SmartServoBridge library in WebSocket mode.
 * Demonstrates how to use the library to bridge between smart servos and web clients.
 *
 * Hardware Requirements:
 * - ESP32 board
 * - Smart servo (e.g., Dynamixel)
 * - RobotStack Smart Servo Add-On Board, or equivalent
 * - TX_EN connected to D3 (or specify custom pin)
 *
 * Written by Nicholas Stedman (nick@robotstack.com)
 */

#include <SmartServoBridge.h>

#define IO Serial

// WiFi credentials
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// Create bridge instance with default settings
SmartServoBridge bridge;

// Callback for text messages
void handleTextMessage(const char *message)
{
    IO.print("Received text message: ");
    IO.println(message);

    // Send echo reply back over WebSocket
    String reply = "{\"type\":\"echo\",\"message\":\"" + String(message) + "\"}";
    bridge.sendTextMessage(reply);
}

void setup()
{
    // Initialize LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH); // Start with LED off (active-low)

    // Initialize Serial for debug output
    IO.begin(1000000);
    delay(1000);

    IO.println("Starting SmartServoBridge in WebSocket mode...");

    // Enable debug output BEFORE initializing the bridge to log connection details
    // bridge.enableDebug(&IO);

    // Set up the bridge
    bridge.begin(ssid, password); // Initialize in WebSocket mode
    bridge.setTextMessageHandler(handleTextMessage);

    // Wait for WiFi connection with LED blinking
    IO.println("Waiting for WiFi connection...");
    unsigned long lastBlink = 0;
    bool ledState = false;

    while (!bridge.isConnected())
    {
        // Flash LED continuously (every 500ms)
        if (millis() - lastBlink >= 500)
        {
            ledState = !ledState;
            digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
            lastBlink = millis();
        }

        // Call bridge.update() to process any connection changes
        bridge.update();
        delay(100);
    }

    if (bridge.isConnected())
    {
        // Wait a moment for IP to be fully available
        delay(100);

        // Get IP address and verify it's valid
        IPAddress ip = bridge.getLocalIP();
        if (ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0)
        {
            IO.println("Error: No valid IP address received");
            ESP.restart();
        }

        // Connection successful - turn LED on and keep it on
        IO.println();
        IO.println("Connected!");
        IO.println("Wifi RSSI: " + String(bridge.getRSSI()) + " dBm");
        IO.print("Connect to the ESP32's IP address (");
        IO.print(ip);
        IO.println(") on port 8080");
        IO.print("e.g. ws://");
        IO.print(ip);
        IO.println(":8080");
        digitalWrite(LED_BUILTIN, LOW);
    }
}

void loop()
{
    bridge.update();

    if (!bridge.isConnected())
    {
        IO.println("Bridge disconnected! Restarting...");
        ESP.restart();
    }
}
