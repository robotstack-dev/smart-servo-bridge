/*
 * SerialBridge.ino
 *
 * Example sketch for SmartServoBridge library in Serial mode.
 * Demonstrates simple data relay between serial port and servo bus.
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

void setup()
{
    // Initialize Serial for bridge communication
    IO.begin(1000000);
    while (!IO)
        delay(10);

    // Use Serial for bridge communication
    bridge.begin(&IO);
    bridge.enableDebug(&IO); // Enable debug output
}

void loop()
{
    bridge.update();
}