/*
 * WebsocketBridge.ino
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
const char *ssid = "Jelly";
const char *password = "J4mmaJ4mma";

// Create bridge instance with default settings
SmartServoBridge bridge;

// Connection status tracking
bool lastConnectionState = false;
unsigned long lastStatusCheck = 0;
unsigned long connectionStartTime = 0;

void testWiFi()
{
    // Scan for available networks
    IO.println("Scanning for WiFi networks...");
    int numNetworks = WiFi.scanNetworks();
    IO.print("Found ");
    IO.print(numNetworks);
    IO.println(" networks:");

    for (int i = 0; i < numNetworks; i++)
    {
        IO.print(i + 1);
        IO.print(": ");
        IO.print(WiFi.SSID(i));
        IO.print(" (");
        IO.print(WiFi.RSSI(i));
        IO.print(" dBm) ");
        IO.print(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "open" : "encrypted");
        IO.println();
    }

    // Check if our target network is found
    bool foundTarget = false;
    for (int i = 0; i < numNetworks; i++)
    {
        if (WiFi.SSID(i) == ssid)
        {
            foundTarget = true;
            IO.print("Found target network '");
            IO.print(ssid);
            IO.print("' with signal strength: ");
            IO.print(WiFi.RSSI(i));
            IO.println(" dBm");
            break;
        }
    }

    if (!foundTarget)
    {
        IO.print("WARNING: Target network '");
        IO.print(ssid);
        IO.println("' not found in scan!");
    }

    // Simple connection test
    IO.println("Testing basic WiFi connection...");
    WiFi.begin(ssid, password);

    int attempts = 0;
    const int maxAttempts = 10;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts)
    {
        delay(1000);
        attempts++;
        IO.print("Attempt ");
        IO.print(attempts);
        IO.print("/");
        IO.print(maxAttempts);
        IO.print(" - Status: ");
        IO.println(WiFi.status());
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        IO.println("Basic WiFi connection successful!");
        IO.print("IP: ");
        IO.println(WiFi.localIP());
        IO.print("RSSI: ");
        IO.println(WiFi.RSSI());
    }
    else
    {
        IO.println("Basic WiFi connection failed!");
        IO.print("Final status: ");
        IO.println(WiFi.status());
    }
}

// Callback for text messages
void handleTextMessage(const char *message)
{
    IO.print("Received text message: ");
    IO.println(message);

    // Echo back the message to test WebSocket functionality
    // Note: This is a temporary test - the bridge doesn't normally echo messages
    IO.println("Echoing message back to client...");
}

void printConnectionStatus()
{
    bool currentState = bridge.isConnected();

    if (currentState != lastConnectionState)
    {
        if (currentState)
        {
            IO.println("\n=== CONNECTION ESTABLISHED ===");
            IO.print("IP Address: ");
            IO.println(bridge.getLocalIP());
            IO.print("RSSI: ");
            IO.print(bridge.getRSSI());
            IO.println(" dBm");
            IO.println("WebSocket server running on port 8080");
            IO.println("Connect to: ws://" + bridge.getLocalIP().toString() + ":8080");
            IO.println("==============================\n");
            connectionStartTime = millis();
        }
        else
        {
            IO.println("\n=== CONNECTION LOST ===");
            if (connectionStartTime > 0)
            {
                unsigned long uptime = (millis() - connectionStartTime) / 1000;
                IO.print("Connection was stable for ");
                IO.print(uptime);
                IO.println(" seconds");
            }
            IO.println("=====================\n");
        }
        lastConnectionState = currentState;
    }

    // Print periodic status every 5 seconds
    if (millis() - lastStatusCheck >= 5000)
    {
        lastStatusCheck = millis();

        if (currentState)
        {
            IO.print("Status: Connected | IP: ");
            IO.print(bridge.getLocalIP());
            IO.print(" | RSSI: ");
            IO.print(bridge.getRSSI());
            IO.print(" dBm | Uptime: ");
            IO.print((millis() - connectionStartTime) / 1000);
            IO.println("s");
        }
        else
        {
            IO.print("Status: Disconnected | WiFi Status: ");
            IO.print(WiFi.status());
            IO.print(" | RSSI: ");
            IO.println(WiFi.RSSI());
        }
    }
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
    IO.println("Enhanced debugging enabled");

    // testWiFi();

    // Enable debug output BEFORE initializing the bridge
    bridge.enableDebug(&IO);

    // Test debug output
    IO.println("Testing debug output...");
    if (bridge.isDebugAvailable())
    {
        IO.println("Debug system is working!");
    }
    else
    {
        IO.println("WARNING: Debug system is NOT working!");
    }

    // Set up the bridge
    IO.println("Initializing bridge...");
    bridge.begin(ssid, password); // Initialize in WebSocket mode
    bridge.setTextMessageHandler(handleTextMessage);

    // Wait for WiFi connection with LED blinking
    IO.println("Waiting for WiFi connection...");
    unsigned long startTime = millis();
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

        // Print connection status
        printConnectionStatus();

        // Add some debug info about WiFi status
        static unsigned long lastWiFiCheck = 0;
        if (millis() - lastWiFiCheck >= 2000)
        {
            lastWiFiCheck = millis();
            IO.print("WiFi Status: ");
            IO.print(WiFi.status());
            IO.print(" | RSSI: ");
            IO.print(WiFi.RSSI());
            IO.print(" dBm | Bridge Connected: ");
            IO.println(bridge.isConnected() ? "YES" : "NO");
        }

        delay(100);
    }

    if (bridge.isConnected())
    {
        // Connection successful - turn LED on and keep it on
        IO.println();
        IO.println("Bridge started in WebSocket mode");
        IO.print("Connect to the ESP32's IP address (");
        IO.print(bridge.getLocalIP());
        IO.println(") on port 8080");
        IO.print("WiFi RSSI: ");
        IO.print(bridge.getRSSI());
        IO.println(" dBm");
        digitalWrite(LED_BUILTIN, LOW);
    }
}

void loop()
{
    bridge.update();

    // Print connection status
    printConnectionStatus();

    if (!bridge.isConnected())
    {
        IO.println("Bridge disconnected! Restarting...");
        ESP.restart();
    }
}
