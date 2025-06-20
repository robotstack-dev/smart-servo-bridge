#include "SmartServoBridge.h"

// Initialize static member
SmartServoBridge *SmartServoBridge::_instance = nullptr;

// Constants
#define WIFI_CONNECT_TIMEOUT 20000 // 20 seconds timeout
#define WIFI_RECONNECT_DELAY 5000  // 5 seconds between reconnect attempts
#define MIN_WIFI_RSSI -80          // More lenient signal strength threshold

/**
 * @brief Construct a new SmartServoBridge object.
 * @param servoBaudRate Baud rate for the servo bus
 * @param txEnablePin Pin to enable TX for half-duplex
 * @param rxPin RX pin for the servo bus
 * @param txPin TX pin for the servo bus
 */
SmartServoBridge::SmartServoBridge(uint32_t servoBaudRate, uint8_t txEnablePin, int8_t rxPin, int8_t txPin)
    : _servoBaudRate(servoBaudRate), _txEnablePin(txEnablePin), _rxPin(rxPin), _txPin(txPin)
{
    _instance = this;
}

/**
 * @brief Set the serial port for debug output and/or serial relay mode
 * @param port Pointer to a Stream object
 */
void SmartServoBridge::setSerialPort(Stream *port)
{
    _serialPort = port;
}

/**
 * @brief Initialize the library in WebSocket mode
 * @param ssid WiFi SSID
 * @param password WiFi password
 */
void SmartServoBridge::begin(const char *ssid, const char *password)
{
    if (_isInitialized)
    {
        if (isDebugAvailable())
        {
            _debugPort->println("[log] Error: Bridge already initialized");
        }
        return;
    }

    _ssid = ssid;
    _password = password;
    _useSerial = false;

    // Initialize pins
    pinMode(_txEnablePin, OUTPUT);
    digitalWrite(_txEnablePin, LOW);

    // Initialize serial communication
    if (isDebugAvailable())
    {
        _debugPort->print("[log] Initializing serial port at ");
        _debugPort->println(_servoBaudRate);
        _debugPort->print("[log] RX: ");
        _debugPort->println(_rxPin);
        _debugPort->print("[log] TX: ");
        _debugPort->println(_txPin);
        _debugPort->print("[log] TX_EN: ");
        _debugPort->println(_txEnablePin);
    }

    _servoBus.begin(_servoBaudRate, SERIAL_8N1, _rxPin, _txPin);

    // Setup networking
    _setupNetworking();
    _isInitialized = true;
}

/**
 * @brief Initialize the library in Serial mode
 * @param bridgePort Stream for bridge communication
 */
void SmartServoBridge::begin(Stream *bridgePort)
{
    if (_isInitialized)
    {
        if (isDebugAvailable())
        {
            _debugPort->println("[log] Error: Bridge already initialized");
        }
        return;
    }

    if (!bridgePort)
    {
        if (isDebugAvailable())
        {
            _debugPort->println("[log] Error: Bridge port not set");
        }
        return;
    }

    _useSerial = true;
    _bridgePort = bridgePort;

    // Initialize pins
    pinMode(_txEnablePin, OUTPUT);
    digitalWrite(_txEnablePin, LOW);

    // Initialize serial communication
    if (isDebugAvailable())
    {
        _debugPort->print("[log] Initializing serial port at ");
        _debugPort->println(_servoBaudRate);
        _debugPort->print("[log] RX: ");
        _debugPort->println(_rxPin);
        _debugPort->print("[log] TX: ");
        _debugPort->println(_txPin);
        _debugPort->print("[log] TX_EN: ");
        _debugPort->println(_txEnablePin);
    }

    _servoBus.begin(_servoBaudRate, SERIAL_8N1, _rxPin, _txPin);
    _isInitialized = true;
}

/**
 * @brief Enable debug output on the specified port
 * @param debugPort Stream for debug output
 * @param enable true to enable, false to disable
 */
void SmartServoBridge::enableDebug(Stream *debugPort, bool enable)
{
    _debugPort = debugPort;
    _debugEnabled = enable;
}

/**
 * @brief Disable debug output
 */
void SmartServoBridge::disableDebug()
{
    _debugEnabled = false;
    _debugPort = nullptr;
}

/**
 * @brief Check if debug output is currently available
 * @return true if debug output is safe to use
 */
bool SmartServoBridge::isDebugAvailable()
{
    return _debugEnabled && _debugPort;
}

/**
 * @brief Main loop function. Call this frequently in your main loop.
 */
void SmartServoBridge::update()
{
    if (!_isInitialized)
    {
        return;
    }

    if (_useSerial)
    {
        _handleSerialData();
        _relayFromServo(); // Add this to handle servo responses in serial mode
    }
    else
    {
        // Check if WiFi connection state has changed
        if (!_wifiConnected && WiFi.status() == WL_CONNECTED)
        {
            // WiFi just connected
            _wifiConnected = true;
            if (isDebugAvailable())
            {
                _debugPort->print("[log] Connected to WiFi with IP: ");
                _debugPort->println(WiFi.localIP());
                _debugPort->print("[log] Signal strength (RSSI): ");
                _debugPort->print(WiFi.RSSI());
                _debugPort->println(" dBm");
                _debugPort->println("[log] Ready! Use port 8080 to connect.");
            }
        }
        else if (_wifiConnected && WiFi.status() != WL_CONNECTED)
        {
            // WiFi connection lost
            if (isDebugAvailable())
            {
                _debugPort->println("[log] Connection lost");
                _debugPort->print("[log] Last known RSSI: ");
                _debugPort->print(WiFi.RSSI());
                _debugPort->println(" dBm");
            }
            _wifiConnected = false;
        }

        // Handle WebSocket operations if connected
        if (_wifiConnected)
        {
            if (WiFi.RSSI() < MIN_WIFI_RSSI)
            {
                if (isDebugAvailable())
                {
                    _debugPort->print("[log] Warning: Weak signal (RSSI: ");
                    _debugPort->print(String(WiFi.RSSI()).c_str());
                    _debugPort->println(" dBm)");
                }
                // Don't disconnect on weak signal, just warn
            }

            // Call WebSocket loop to handle connections
            _webSocket.loop();

            // Only relay from servo if we have data
            if (_servoBus.available() > 0)
            {
                _relayFromServo();
            }
        }
        else
        {
            // Try to reconnect if enough time has passed and not already attempting
            static unsigned long lastReconnectAttempt = 0;
            static bool reconnectAttempted = false;

            if (millis() - lastReconnectAttempt > WIFI_RECONNECT_DELAY && !reconnectAttempted)
            {
                lastReconnectAttempt = millis();
                reconnectAttempted = true;
                if (isDebugAvailable())
                {
                    _debugPort->println("[log] Attempting WiFi reconnection...");
                }
                _setupNetworking();
            }

            // Reset reconnect flag if we get connected
            if (WiFi.status() == WL_CONNECTED)
            {
                reconnectAttempted = false;
            }
        }
    }
}

/**
 * @brief Handle serial data in serial mode
 */
void SmartServoBridge::_handleSerialData()
{
    if (!_bridgePort)
    {
        return;
    }

    // Read all available data into a buffer
    size_t available = _bridgePort->available();
    if (available > 0)
    {
        // Check if the data looks like text (all printable ASCII)
        bool isText = true;
        uint8_t buffer[available];
        size_t bytesRead = _bridgePort->readBytes(buffer, available);

        if (bytesRead > 0)
        {
            // Check if all bytes are printable ASCII
            for (size_t i = 0; i < bytesRead; i++)
            {
                if (buffer[i] < 32 || buffer[i] > 126)
                {
                    isText = false;
                    break;
                }
            }

            if (isText && _textMessageHandler)
            {
                // Null terminate the text for string handling
                char *text = new char[bytesRead + 1];
                memcpy(text, buffer, bytesRead);
                text[bytesRead] = '\0';

                if (isDebugAvailable())
                {
                    _debugPort->print("[log] Received text command: ");
                    _debugPort->println(text);
                }

                _textMessageHandler(text);
                delete[] text;
            }
            else
            {
                if (isDebugAvailable())
                {
                    _debugPort->print("[log] Received binary data (");
                    _debugPort->print(bytesRead);
                    _debugPort->println(" bytes)");
                }

                // Relay the binary data to the servo
                _relayToServo(buffer, bytesRead);
            }
        }
    }
}

/**
 * @brief Set a callback for when a command is sent to the servo.
 * @param handler Function pointer to the callback
 */
void SmartServoBridge::setServoCommandHandler(void (*handler)(uint8_t *, size_t))
{
    _servoCommandHandler = handler;
}

/**
 * @brief Set a callback for when a response is received from the servo.
 * @param handler Function pointer to the callback
 */
void SmartServoBridge::setServoResponseHandler(void (*handler)(uint8_t *, size_t))
{
    _servoResponseHandler = handler;
}

/**
 * @brief Set a callback for when a text message is received over WebSocket.
 * @param handler Function pointer to the callback
 */
void SmartServoBridge::setTextMessageHandler(void (*handler)(const char *))
{
    _textMessageHandler = handler;
}

/**
 * @brief Check if WiFi is connected.
 * @return true if connected, false otherwise
 */
bool SmartServoBridge::isConnected()
{
    if (_wifiConnected)
    {
        return true;
    }

    // If not connected, provide detailed status information only when status changes
    int wifiStatus = WiFi.status();
    static int lastWifiStatus = -1;

    if (wifiStatus != lastWifiStatus && isDebugAvailable())
    {
        lastWifiStatus = wifiStatus;
        _debugPort->print("[log] WiFi status: ");
        _debugPort->print(wifiStatus);
        _debugPort->print(" (");
        switch (wifiStatus)
        {
        case WL_IDLE_STATUS:
            _debugPort->print("WL_IDLE_STATUS");
            break;
        case WL_NO_SSID_AVAIL:
            _debugPort->print("WL_NO_SSID_AVAIL");
            break;
        case WL_SCAN_COMPLETED:
            _debugPort->print("WL_SCAN_COMPLETED");
            break;
        case WL_CONNECTED:
            _debugPort->print("WL_CONNECTED");
            break;
        case WL_CONNECT_FAILED:
            _debugPort->print("WL_CONNECT_FAILED");
            break;
        case WL_CONNECTION_LOST:
            _debugPort->print("WL_CONNECTION_LOST");
            break;
        case WL_DISCONNECTED:
            _debugPort->print("WL_DISCONNECTED");
            break;
        default:
            _debugPort->print("Unknown");
            break;
        }
        _debugPort->println(")");

        // Show error list on first failure detection
        static bool showedErrorList = false;
        if (wifiStatus == WL_CONNECT_FAILED && !showedErrorList)
        {
            showedErrorList = true;
            _debugPort->println("[log] Connection failed! Possible issues:");
            _debugPort->println("[log] 1. Incorrect SSID or password");
            _debugPort->println("[log] 2. WiFi network not in range");
            _debugPort->println("[log] 3. Network requires additional authentication");
            _debugPort->println("[log] 4. Power supply insufficient for WiFi operation");
            _debugPort->println("[log] 5. Network is 5GHz only (ESP32 supports 2.4GHz)");
        }
    }

    return false;
}

/**
 * @brief Get the local IP address.
 * @return IPAddress object
 */
IPAddress SmartServoBridge::getLocalIP()
{
    return WiFi.localIP();
}

/**
 * @brief Get the current WiFi RSSI (signal strength).
 * @return RSSI in dBm
 */
int SmartServoBridge::getRSSI()
{
    return WiFi.RSSI();
}

/**
 * @brief Setup WiFi and WebSocket networking.
 */
void SmartServoBridge::_setupNetworking()
{
    if (isDebugAvailable())
    {
        _debugPort->println("[log] Setting up networking...");
    }

    // Check current WiFi status
    int currentStatus = WiFi.status();
    if (isDebugAvailable())
    {
        _debugPort->print("[log] Current WiFi status: ");
        _debugPort->println(currentStatus);
    }

    // Only configure WiFi if not already connected or connecting
    if (currentStatus != WL_CONNECTED && currentStatus != WL_IDLE_STATUS)
    {
        if (isDebugAvailable())
        {
            _debugPort->println("[log] Configuring WiFi...");
        }

        // Configure WiFi
        WiFi.onEvent(_wifiEvent);
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.persistent(false);
        WiFi.setSleep(false);                // Disable WiFi sleep mode for better stability
        WiFi.setTxPower(WIFI_POWER_19_5dBm); // Set maximum power

        // Begin connection (non-blocking)
        WiFi.begin(_ssid, _password);
    }
    else if (currentStatus == WL_CONNECTED)
    {
        if (isDebugAvailable())
        {
            _debugPort->print("[log] WiFi already connected with IP: ");
            _debugPort->println(WiFi.localIP());
        }
        _wifiConnected = true;
    }
    else
    {
        if (isDebugAvailable())
        {
            _debugPort->println("[log] WiFi is already connecting, waiting...");
        }
    }
}

/**
 * @brief Relay data to the servo bus.
 * @param mem Pointer to data
 * @param len Length of data
 */
void SmartServoBridge::_relayToServo(const void *mem, uint32_t len)
{
    const uint8_t *src = (const uint8_t *)mem;

    if (_debugPort)
    {
        _debugPort->print("[log] Relaying ");
        _debugPort->print(len);
        _debugPort->println(" bytes to servo");
    }

    if (_debugPort)
    {
        _debugPort->print("[log] Sending to servo (");
        _debugPort->print(len);
        _debugPort->println(" bytes):");
        for (uint32_t i = 0; i < len; i++)
        {
            _debugPort->printf("%02X ", src[i]);
        }
        _debugPort->println();
    }

    digitalWrite(_txEnablePin, HIGH);
    _servoBus.write(src, len);
    _servoBus.flush();
    delayMicroseconds(_waitForFlushComplete);
    digitalWrite(_txEnablePin, LOW);

    if (_servoCommandHandler)
    {
        _servoCommandHandler((uint8_t *)mem, len);
    }
}

/**
 * @brief Relay data from the servo bus.
 */
void SmartServoBridge::_relayFromServo()
{
    while (_servoBus.available())
    {
        long len = _servoBus.available();
        uint8_t buffer[len];
        _servoBus.readBytes(buffer, len);

        if (_debugPort)
        {
            _debugPort->println("\n[log] Reply from servo:");
            for (int i = 0; i < len; i++)
            {
                _debugPort->printf("%02X ", buffer[i]);
            }
            _debugPort->println();
        }

        if (!_useSerial)
        {
            // Send to first connected client (client 0)
            // The WebSocket library will handle invalid client numbers gracefully
            _webSocket.sendBIN(0, buffer, len);
        }
        else if (_bridgePort)
        {
            _bridgePort->write(buffer, len);
        }

        if (_servoResponseHandler)
        {
            _servoResponseHandler(buffer, len);
        }

        if (_debugPort)
        {
            _debugPort->print("[log] Received ");
            _debugPort->print(len);
            _debugPort->println(" bytes from servo");
        }
    }
}

/**
 * @brief Handle WebSocket events.
 * @param num Client number
 * @param type Event type
 * @param payload Data payload
 * @param length Length of payload
 */
void SmartServoBridge::_webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    if (isDebugAvailable())
    {
        _debugPort->print("[log] WebSocket event: ");
        _debugPort->print(type);
        _debugPort->print(" (client ");
        _debugPort->print(num);
        _debugPort->println(")");
    }

    switch (type)
    {
    case WStype_DISCONNECTED:
        if (isDebugAvailable())
        {
            _debugPort->printf("[log] [%u] Disconnected!\n", num);
        }
        break;

    case WStype_CONNECTED:
        if (isDebugAvailable())
        {
            IPAddress ip = _webSocket.remoteIP(num);
            _debugPort->printf("[log] [%u] Connected from %d.%d.%d.%d\n",
                               num, ip[0], ip[1], ip[2], ip[3]);
        }
        break;

    case WStype_TEXT:
        if (_textMessageHandler)
        {
            // Null terminate the payload for string handling
            char *text = new char[length + 1];
            memcpy(text, payload, length);
            text[length] = '\0';

            if (isDebugAvailable())
            {
                _debugPort->printf("[log] [%u] Received text: %s\n", num, text);
            }

            // Handle ping/pong for testing
            if (strcmp(text, "ping") == 0)
            {
                if (isDebugAvailable())
                {
                    _debugPort->printf("[log] [%u] Sending pong response\n", num);
                }
                _webSocket.sendTXT(num, "pong");
            }
            else
            {
                _textMessageHandler(text);
            }

            delete[] text;
        }
        break;

    case WStype_BIN:
        _relayToServo(payload, length);
        if (isDebugAvailable())
        {
            _debugPort->printf("[log] [%u] Received binary length: %u\n", num, length);
        }
        break;

    default:
        break;
    }
}

/**
 * @brief Handle WiFi events (static).
 * @param event WiFi event ID
 */
void SmartServoBridge::_wifiEvent(arduino_event_id_t event)
{
    if (!_instance || !_instance->isDebugAvailable())
        return;

    // Only log state changes
    static arduino_event_id_t lastEvent = ARDUINO_EVENT_MAX;
    if (event == lastEvent)
        return;
    lastEvent = event;

    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_START:
        _instance->_debugPort->println("[log] WiFi client started");
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        _instance->_debugPort->println("[log] WiFi client stopped");
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        _instance->_debugPort->println("[log] Connected to WiFi");
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        _instance->_debugPort->println("[log] Disconnected from WiFi");
        _instance->_wifiConnected = false;
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        _instance->_debugPort->print("[log] Got IP: ");
        _instance->_debugPort->println(WiFi.localIP());
        _instance->_wifiConnected = true;

        // Start WebSocket server now that we have an IP
        if (!_instance->_useSerial)
        {
            _instance->_debugPort->println("[log] Starting WebSocket server...");
            _instance->_webSocket.begin();
            _instance->_webSocket.onEvent([_instance](uint8_t num, WStype_t type, uint8_t *payload, size_t length)
                                          { _instance->_webSocketEvent(num, type, payload, length); });
            _instance->_debugPort->println("[log] WebSocket server started on port 8080");
            _instance->_debugPort->print("[log] Connect to: ws://");
            _instance->_debugPort->print(WiFi.localIP());
            _instance->_debugPort->println(":8080");
            _instance->_debugPort->println("[log] WebSocket server should now be accepting connections");
        }
        break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        _instance->_debugPort->println("[log] Lost IP address");
        _instance->_wifiConnected = false;
        break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
        _instance->_debugPort->println("[log] WiFi authentication mode changed");
        break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
        _instance->_debugPort->println("[log] WiFi WPS succeeded");
        break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
        _instance->_debugPort->println("[log] WiFi WPS failed");
        break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
        _instance->_debugPort->println("[log] WiFi WPS timeout");
        break;
    case ARDUINO_EVENT_WPS_ER_PIN:
        _instance->_debugPort->println("[log] WiFi WPS PIN");
        break;
    }
}

/**
 * @brief Relay data to the servo bus.
 * @param mem Pointer to data
 * @param len Length of data
 */
void SmartServoBridge::relayToServo(const void *mem, uint32_t len)
{
    _relayToServo(mem, len);
}