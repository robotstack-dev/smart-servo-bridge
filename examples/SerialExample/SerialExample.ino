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

// Create bridge instance with 1M baud rate
SmartServoBridge bridge(1000000);

// Callback for text commands
void handleTextCommand(const char *text)
{
    Serial.print("Received text command: ");
    Serial.println(text);

    // Example: Handle different text commands
    if (strcmp(text, "help") == 0)
    {
        Serial.println("Available commands:");
        Serial.println("  help   - Show available commands");
        Serial.println("  ping   - Send ping command to servo");
    }
    else if (strcmp(text, "ping") == 0)
    {
        // Example: Send a ping command to the servo (ID 1)
        // Header(2) + ID(1) + Length(1) + Instruction(1) + Checksum(1)
        uint8_t pingCmd[] = {0xFF, 0xFF, 0x01, 0x02, 0x01, 0xFB};
        bridge.relayToServo(pingCmd, sizeof(pingCmd));
    }
    else
    {
        Serial.println("Unknown command. Type 'help' for available commands.");
    }
}

// Callback for servo responses
void handleServoResponse(uint8_t *data, size_t len)
{
    Serial.print("Received response (");
    Serial.print(len);
    Serial.println(" bytes):");
    for (size_t i = 0; i < len; i++)
    {
        Serial.printf("%02X ", data[i]);
    }
    Serial.println();

    // Check if this is a ping response
    if (len == 6 && data[0] == 0xFF && data[1] == 0xFF && data[2] == 0x01 && data[3] == 0x02)
    {
        if (data[4] == 0x00)
        {
            Serial.println("Ping successful - servo is responding");
        }
        else
        {
            Serial.print("Ping error: 0x");
            Serial.println(data[4], HEX);
        }
    }
}

void setup()
{
    // Initialize Serial for both bridge communication and debug output
    Serial.begin(1000000);
    while (!Serial)
        delay(10);

    Serial.println("Starting bridge...");

    // Use Serial for both bridge communication and debug output
    bridge.begin(&Serial);       // Use Serial for bridge communication
    bridge.enableDebug(&Serial); // Enable debug output

    // Set up handlers
    bridge.setTextMessageHandler(handleTextCommand);
    bridge.setServoResponseHandler(handleServoResponse);

    Serial.println("Bridge started in Serial mode");
    Serial.println("Send data to the ESP32's serial port to relay to the servo");
    Serial.println("Format:");
    Serial.println("  - Send raw bytes to communicate with the servo");
    Serial.println("  - Send text commands for high-level control");
    Serial.println("Example text commands:");
    Serial.println("  help   - Show available commands");
    Serial.println("  ping   - Send ping command to servo");
}

void loop()
{
    bridge.update();
}