/*
 * SerialExample.ino
 *
 * Example sketch for SmartServoBridge library in Serial mode.
 * Demonstrates how to use the library to bridge between smart servos and serial port.
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

void setup()
{
    // Initialize Serial for both bridge communication and debug output
    Serial.begin(1000000);
    while (!Serial)
        delay(10);

    // Use Serial for both bridge communication and debug output
    bridge.begin(&Serial);       // Use Serial for bridge communication
    bridge.enableDebug(&Serial); // Also use Serial for debug output

    Serial.println("Bridge started in Serial mode");
    Serial.println("Send data to the ESP32's serial port to relay to the servo");
}

void loop()
{
    bridge.update();
}