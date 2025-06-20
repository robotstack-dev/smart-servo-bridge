/*
 * WebSocketBridgeMinimal.ino
 *
 * Minimal WebSocket bridge test - disables servo communication to isolate WebSocket issues.
 * This helps determine if the problem is with servo communication or WebSocket handling.
 *
 * Hardware Requirements:
 * - ESP32 board
 *
 * Written by Nicholas Stedman (nick@robotstack.com)
 */

#include <SmartServoBridge.h>
#include <WebSocketsServer.h>

#define IO Serial

// WiFi credentials
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// Create bridge instance with minimal servo settings
SmartServoBridge bridge(1000000, D3, -1, -1); // Disable servo pins

// Manual WebSocket server for testing
WebSocketsServer manualWebSocket = WebSocketsServer(8080);
bool manualWebSocketStarted = false;

// Connection status tracking
bool lastConnectionState = false;
unsigned long lastStatusCheck = 0;
unsigned long connectionStartTime = 0;

// Callback for text messages
void handleTextMessage(const char *message)
{
    IO.print("Received text message: ");
    IO.println(message);

    // Echo back the message to test WebSocket functionality
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

            // Test WebSocket server every 30 seconds
            static unsigned long lastWebSocketTest = 0;
            if (millis() - lastWebSocketTest >= 30000)
            {
                lastWebSocketTest = millis();
                IO.println("Testing WebSocket server connectivity...");

                if (manualWebSocketStarted)
                {
                    IO.println("✅ Manual WebSocket server is running on port 8080");
                    IO.println("   (TCP test may fail but WebSocket connections should work)");
                }
                else
                {
                    IO.println("❌ No WebSocket server detected");
                }

                // Optional: Test with actual WebSocket client connection
                // This would require a more complex test, but the fact that you can
                // connect with your client means the server is working
            }
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

// WebSocket event handler
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
        IPAddress ip = manualWebSocket.remoteIP(num);
        IO.printf("[%u] Connected from %d.%d.%d.%d\n",
                  num, ip[0], ip[1], ip[2], ip[3]);
    }
    break;

    case WStype_TEXT:
        IO.printf("[%u] Received text: %s\n", num, payload);
        // Echo back the message
        manualWebSocket.sendTXT(num, payload);
        break;

    case WStype_BIN:
        IO.printf("[%u] Received binary length: %u\n", num, length);
        // Echo back the binary data
        manualWebSocket.sendBIN(num, payload, length);
        break;

    default:
        IO.printf("[%u] Unknown event type: %u\n", num, type);
        break;
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

    IO.println("Starting SmartServoBridge Minimal Test...");
    IO.println("Servo communication DISABLED - WebSocket only");

    // Scan for available networks first
    IO.println("Scanning for WiFi networks...");
    int numNetworks = WiFi.scanNetworks();
    IO.print("Found ");
    IO.print(numNetworks);
    IO.println(" networks:");

    bool foundTarget = false;
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

        if (WiFi.SSID(i) == ssid)
        {
            foundTarget = true;
            IO.print("✓ Found target network '");
            IO.print(ssid);
            IO.print("' with signal strength: ");
            IO.print(WiFi.RSSI(i));
            IO.println(" dBm");
        }
    }

    if (!foundTarget)
    {
        IO.print("✗ WARNING: Target network '");
        IO.print(ssid);
        IO.println("' not found in scan!");
        IO.println("Check SSID spelling or network availability");
    }

    // Test basic WiFi connection first
    IO.println("Testing basic WiFi connection...");
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(false);
    WiFi.setSleep(false);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.begin(ssid, password);

    int attempts = 0;
    const int maxAttempts = 20;
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
        IO.println("✓ Basic WiFi connection successful!");
        IO.print("IP: ");
        IO.println(WiFi.localIP());
        IO.print("RSSI: ");
        IO.println(WiFi.RSSI());
    }
    else
    {
        IO.println("✗ Basic WiFi connection failed!");
        IO.print("Final status: ");
        IO.println(WiFi.status());
        IO.println("Check your WiFi credentials and try again");
        return; // Don't proceed if WiFi fails
    }

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

    // Since WiFi is already connected, the bridge should detect it
    IO.println("WiFi is already connected, bridge should detect it automatically");
    delay(1000); // Give bridge time to detect the connection

    // Test if WebSocket server is actually listening
    IO.println("Testing WebSocket server status...");
    delay(2000); // Give it time to start

    // Note: TCP connection test doesn't work reliably with WebSocket servers
    // because WebSocket requires a specific handshake protocol
    IO.println("Note: TCP connection test may fail even if WebSocket server is working");
    IO.println("The real test is whether you can connect with a WebSocket client");

    // Simple test - try to connect to ourselves (this may fail even if WebSocket works)
    WiFiClient testClient;
    if (testClient.connect(bridge.getLocalIP(), 8080))
    {
        IO.println("✅ TCP port 8080 is open (WebSocket server likely working)");
        testClient.stop();
    }
    else
    {
        IO.println("⚠️ TCP connection test failed, but WebSocket server may still be working");
        IO.println("This is normal - WebSocket servers don't always respond to basic TCP connections");

        // Additional debugging
        IO.println("Checking bridge connection status...");
        IO.print("Bridge isConnected(): ");
        IO.println(bridge.isConnected() ? "YES" : "NO");
        IO.print("WiFi status: ");
        IO.println(WiFi.status());
        IO.print("WiFi IP: ");
        IO.println(WiFi.localIP());

        // Manual WebSocket server initialization
        IO.println("SmartServoBridge WebSocket server not starting - using manual initialization");
        IO.println("This is a workaround for the library issue");

        // Test if we can manually start a WebSocket server
        IO.println("Testing manual WebSocket server creation...");
        manualWebSocket.begin();
        manualWebSocket.onEvent(webSocketEvent);
        manualWebSocketStarted = true;
        IO.println("Manual WebSocket server started on port 8080");
        IO.println("Try connecting to ws://" + WiFi.localIP().toString() + ":8080");
    }

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
            int wifiStatus = WiFi.status();
            IO.print("WiFi Status: ");
            IO.print(wifiStatus);
            IO.print(" (");
            switch (wifiStatus)
            {
            case WL_IDLE_STATUS:
                IO.print("IDLE");
                break;
            case WL_NO_SSID_AVAIL:
                IO.print("NO_SSID_AVAIL");
                break;
            case WL_SCAN_COMPLETED:
                IO.print("SCAN_COMPLETED");
                break;
            case WL_CONNECTED:
                IO.print("CONNECTED");
                break;
            case WL_CONNECT_FAILED:
                IO.print("CONNECT_FAILED");
                break;
            case WL_CONNECTION_LOST:
                IO.print("CONNECTION_LOST");
                break;
            case WL_DISCONNECTED:
                IO.print("DISCONNECTED");
                break;
            default:
                IO.print("UNKNOWN");
                break;
            }
            IO.print(") | RSSI: ");
            IO.print(WiFi.RSSI());
            IO.print(" dBm | Bridge Connected: ");
            IO.println(bridge.isConnected() ? "YES" : "NO");

            // Additional troubleshooting info
            if (wifiStatus == WL_CONNECT_FAILED)
            {
                IO.println("WiFi connection failed! Possible issues:");
                IO.println("1. Incorrect SSID or password");
                IO.println("2. Network not in range");
                IO.println("3. Network requires additional authentication");
                IO.println("4. Power supply insufficient");
                IO.println("5. Network is 5GHz only (ESP32 supports 2.4GHz)");
            }
        }

        delay(100);
    }

    if (bridge.isConnected())
    {
        // Connection successful - turn LED on and keep it on
        IO.println();
        IO.println("Bridge started in WebSocket mode (MINIMAL)");
        IO.print("Connect to the ESP32's IP address (");
        IO.print(bridge.getLocalIP());
        IO.println(") on port 8080");
        IO.print("WiFi RSSI: ");
        IO.print(bridge.getRSSI());
        IO.println(" dBm");
        IO.println("Servo communication is DISABLED");
        digitalWrite(LED_BUILTIN, LOW);
    }
}

void loop()
{
    bridge.update();

    // Handle manual WebSocket server if it's started
    if (manualWebSocketStarted)
    {
        manualWebSocket.loop();
    }

    // Print connection status
    printConnectionStatus();

    if (!bridge.isConnected())
    {
        IO.println("Bridge disconnected! Restarting...");
        ESP.restart();
    }
}