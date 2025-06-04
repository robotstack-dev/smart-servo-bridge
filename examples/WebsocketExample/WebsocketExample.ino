/*
 * WebsocketExample.ino
 *
 * Example sketch for SmartServoBridge library in WebSocket mode.
 * Demonstrates how to use the library to bridge between smart servos and web clients.
 *
 * Hardware Requirements:
 * - ESP32 board
 * - Smart servo (e.g., Dynamixel)
 * - RS485 level shifter (if needed)
 * - TX_EN connected to D3 (or specify custom pin)
 *
 * Written by Nicholas Stedman (nick@robotstack.com)
 */

#include <SmartServoBridge.h>

// WiFi credentials
const char *ssid = "your_ssid";
const char *password = "your_password";

// Create bridge instance with default settings
SmartServoBridge bridge;

// Callback for servo commands
void handleServoCommand(uint8_t *data, size_t length)
{
    Serial.print("Command sent to servo (");
    Serial.print(length);
    Serial.println(" bytes):");
    for (size_t i = 0; i < length; i++)
    {
        if (data[i] < 0x10)
            Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

// Callback for servo responses
void handleServoResponse(uint8_t *data, size_t length)
{
    Serial.print("Response from servo (");
    Serial.print(length);
    Serial.println(" bytes):");
    for (size_t i = 0; i < length; i++)
    {
        if (data[i] < 0x10)
            Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

// Callback for text messages
void handleTextMessage(const char *message)
{
    Serial.print("Received text message: ");
    Serial.println(message);
}

void setup()
{
    // Initialize Serial for debug output
    Serial.begin(115200);
    while (!Serial)
        delay(10);

    // Set up the bridge
    bridge.begin(ssid, password); // Initialize in WebSocket mode
    bridge.enableDebug(&Serial);  // Enable debug output on Serial
    bridge.setServoCommandHandler(handleServoCommand);
    bridge.setServoResponseHandler(handleServoResponse);
    bridge.setTextMessageHandler(handleTextMessage);

    Serial.println("Bridge started in WebSocket mode");
    Serial.println("Connect to the ESP32's IP address on port 80");
}

void loop()
{
    bridge.update();
}