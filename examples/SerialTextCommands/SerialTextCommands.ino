/*
 * SerialTextCommands.ino
 *
 * Example sketch for SmartServoBridge library in Serial mode.
 * Demonstrates how to use the library to bridge between smart servos and serial port.
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

// Create bridge instance with 1M baud rate
SmartServoBridge bridge(1000000);

// Callback for text commands
void handleTextMessage(const char *text)
{
    IO.print("Received text command: ");
    IO.println(text);

    // Example: Handle different text commands
    if (strcmp(text, "help") == 0)
    {
        IO.println("Available commands:");
        IO.println("  help   - Show available commands");
        IO.println("  ping   - Send ping command to servo");
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
        IO.println("Unknown command. Type 'help' for available commands.");
    }
}

// Callback for servo responses
void handleServoResponse(uint8_t *data, size_t len)
{
    IO.print("Received response (");
    IO.print(len);
    IO.println(" bytes):");
    for (size_t i = 0; i < len; i++)
    {
        IO.printf("%02X ", data[i]);
    }
    IO.println();

    // Check if this is a ping response
    if (len == 6 && data[0] == 0xFF && data[1] == 0xFF && data[2] == 0x01 && data[3] == 0x02)
    {
        if (data[4] == 0x00)
        {
            IO.println("Ping successful - servo is responding");
        }
        else
        {
            IO.print("Ping error: 0x");
            IO.println(data[4], HEX);
        }
    }
}

void setup()
{
    // Initialize Serial for both bridge communication and debug output
    IO.begin(1000000);
    while (!IO)
        delay(10);

    IO.println("Starting bridge...");

    // Use Serial for both bridge communication and debug output
    bridge.begin(&IO); // Use Serial for bridge communication
    // bridge.enableDebug(&IO); // Enable debug output

    // Set up handlers
    bridge.setTextMessageHandler(handleTextMessage);
    bridge.setServoResponseHandler(handleServoResponse);

    IO.println("Bridge started in Serial mode");
    IO.println("Send data to the ESP32's serial port to relay to the servo");
    IO.println("Format:");
    IO.println("  - Send raw bytes to communicate with the servo");
    IO.println("  - Send text commands for high-level control");
    IO.println("Example text commands:");
    IO.println("  help   - Show available commands");
    IO.println("  ping   - Send ping command to servo");
}

void loop()
{
    bridge.update();
}