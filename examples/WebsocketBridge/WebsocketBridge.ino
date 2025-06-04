/*
 * BasicExample.ino
 *
 * Example sketch for the SmartServoWebsocket library.
 * Shows basic usage of the library to control a smart servo over WebSocket.
 *
 * Hardware:
 * - ESP32 development board
 * - Smart servo (e.g., Dynamixel XL430-W250)
 * - Half-duplex UART circuit
 *
 * Wiring:
 * - Connect TX_EN to D3 (or change in code)
 * - Connect servo data line to ESP32's UART pins
 * - Connect servo power and ground
 */

#include <SmartServoWebsocket.h>

// WiFi credentials
const char *ssid = "yourSSID";
const char *password = "yourPassword";

// Create SmartServoWebsocket instance
SmartServoWebsocket servoWebsocket;

// Uncomment to enable debug output
// #define ENABLE_DEBUG

void setup()
{
#ifdef ENABLE_DEBUG
    // Initialize Serial for debugging
    Serial.begin(1000000);
    while (!Serial)
        delay(10);

    // Set debug stream
    servoWebsocket.setDebugStream(Serial);
#endif

    // Optional: Set handlers for servo commands and responses
    servoWebsocket.setServoCommandHandler(handleServoCommand);
    servoWebsocket.setServoResponseHandler(handleServoResponse);

    // Optional: Set text message handler
    servoWebsocket.setTextMessageHandler(handleTextMessage);

    // Initialize the library
    servoWebsocket.begin(ssid, password);
}

void loop()
{
    // Update the library (handles WebSocket and servo communication)
    servoWebsocket.update();
}

// Optional: Handler for servo commands
void handleServoCommand(uint8_t *data, size_t length)
{
    // Add any custom command handling here
}

// Optional: Handler for servo responses
void handleServoResponse(uint8_t *data, size_t length)
{
    // Add any custom response handling here
}

// Optional: Handler for text messages
void handleTextMessage(const char *text)
{
    // Handle the text message
    if (strcmp(text, "status") == 0)
    {
        // Send status info
    }
    else if (strncmp(text, "set ", 4) == 0)
    {
        // Parse and handle set commands
    }
    else if (strncmp(text, "get ", 4) == 0)
    {
        // Parse and handle get commands
    }
}