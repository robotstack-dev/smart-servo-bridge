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
        _debugPrintln("Error: Bridge already initialized");
        return;
    }

    _ssid = ssid;
    _password = password;
    _useSerial = false;

    // Initialize pins
    pinMode(_txEnablePin, OUTPUT);
    digitalWrite(_txEnablePin, LOW);

    // Initialize serial communication
    _debugPrint("Initializing serial port at ");
    _debugPrintln(String(_servoBaudRate).c_str());
    _debugPrint("RX: ");
    _debugPrintln(String(_rxPin).c_str());
    _debugPrint("TX: ");
    _debugPrintln(String(_txPin).c_str());
    _debugPrint("TX_EN: ");
    _debugPrintln(String(_txEnablePin).c_str());

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
        _debugPrintln("Error: Bridge already initialized");
        return;
    }

    if (!bridgePort)
    {
        _debugPrintln("Error: Bridge port not set");
        return;
    }

    _useSerial = true;
    _bridgePort = bridgePort;

    // Initialize pins
    pinMode(_txEnablePin, OUTPUT);
    digitalWrite(_txEnablePin, LOW);

    // Initialize serial communication
    _debugPrint("Initializing serial port at ");
    _debugPrintln(String(_servoBaudRate).c_str());
    _debugPrint("RX: ");
    _debugPrintln(String(_rxPin).c_str());
    _debugPrint("TX: ");
    _debugPrintln(String(_txPin).c_str());
    _debugPrint("TX_EN: ");
    _debugPrintln(String(_txEnablePin).c_str());

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
 * @brief Print debug message if debug is available
 * @param message Message to print
 */
void SmartServoBridge::_debugPrint(const char *message)
{
    if (isDebugAvailable())
    {
        _debugPort->print(message);
    }
}

/**
 * @brief Print debug message with newline if debug is available
 * @param message Message to print
 */
void SmartServoBridge::_debugPrintln(const char *message)
{
    if (isDebugAvailable())
    {
        _debugPort->println(message);
    }
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
    else if (_wifiConnected)
    {
        // Check if we still have a good connection
        if (WiFi.status() != WL_CONNECTED)
        {
            _debugPrintln("Connection lost");
            _wifiConnected = false;
        }
        else if (WiFi.RSSI() < MIN_WIFI_RSSI)
        {
            _debugPrint("Warning: Weak signal (RSSI: ");
            _debugPrint(String(WiFi.RSSI()).c_str());
            _debugPrintln(" dBm)");
            // Don't disconnect on weak signal, just warn
        }
        else
        {
            _webSocket.loop();
            _relayFromServo();
        }
    }
    else
    {
        // Try to reconnect if enough time has passed
        static unsigned long lastReconnectAttempt = 0;
        if (millis() - lastReconnectAttempt > WIFI_RECONNECT_DELAY)
        {
            lastReconnectAttempt = millis();
            _setupNetworking();
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
        uint8_t buffer[available];
        size_t bytesRead = _bridgePort->readBytes(buffer, available);

        if (bytesRead > 0)
        {
            if (_debugPort)
            {
                _debugPort->print("Received ");
                _debugPort->print(bytesRead);
                _debugPort->println(" bytes from bridge port");
            }

            // Relay the entire buffer to the servo
            _relayToServo(buffer, bytesRead);
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
    return _wifiConnected;
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
    if (_serialPort)
    {
        _serialPort->println("Setting up networking...");
    }

    // Only disconnect if we're not already connected
    if (WiFi.status() != WL_CONNECTED)
    {
        // Configure WiFi
        WiFi.onEvent(_wifiEvent);
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.persistent(false);
        WiFi.setSleep(false);                // Disable WiFi sleep mode for better stability
        WiFi.setTxPower(WIFI_POWER_19_5dBm); // Set maximum power

        // Begin connection
        WiFi.begin(_ssid, _password);

        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime < WIFI_CONNECT_TIMEOUT))
        {
            if (_serialPort)
                _serialPort->print('.');
            delay(500);
        }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        int rssi = WiFi.RSSI();
        if (rssi < MIN_WIFI_RSSI && _serialPort)
        {
            _serialPort->print("Warning: Low WiFi signal strength: ");
            _serialPort->print(rssi);
            _serialPort->println(" dBm");
        }

        _wifiConnected = true;
        if (_serialPort)
        {
            _serialPort->print("Connected to WiFi with IP: ");
            _serialPort->println(WiFi.localIP());
            _serialPort->println("Ready! Use port 80 to connect.");
        }

        _webSocket.begin();
        _webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t *payload, size_t length)
                           { this->_webSocketEvent(num, type, payload, length); });
    }
    else
    {
        if (_serialPort)
        {
            _serialPort->println("\nFailed to connect to WiFi!");
            _serialPort->print("Status: ");
            _serialPort->println(WiFi.status());
        }
        _wifiConnected = false;
    }

    if (_serialPort)
    {
        _serialPort->println("WebSocket server started");
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
        _debugPort->print("Relaying ");
        _debugPort->print(len);
        _debugPort->println(" bytes to servo");
    }

    if (_debugPort)
    {
        _debugPort->print("Sending to servo (");
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
            _debugPort->println("\nReply from servo:");
            for (int i = 0; i < len; i++)
            {
                _debugPort->printf("%02X ", buffer[i]);
            }
            _debugPort->println();
        }

        if (!_useSerial)
        {
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
            _debugPort->print("Received ");
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
    if (_serialPort)
    {
        _serialPort->print("WebSocket event: ");
        _serialPort->println(type);
    }

    switch (type)
    {
    case WStype_DISCONNECTED:
        if (_serialPort)
        {
            _serialPort->printf("[%u] Disconnected!\n", num);
        }
        break;

    case WStype_CONNECTED:
        if (_serialPort)
        {
            IPAddress ip = _webSocket.remoteIP(num);
            _serialPort->printf("[%u] Connected from %d.%d.%d.%d\n",
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

            if (_serialPort)
            {
                _serialPort->printf("[%u] Received text: %s\n", num, text);
            }

            _textMessageHandler(text);
            delete[] text;
        }
        break;

    case WStype_BIN:
        _relayToServo(payload, length);
        if (_serialPort)
        {
            _serialPort->printf("[%u] Received binary length: %u\n", num, length);
        }
        break;

    default:
        break;
    }

    if (_serialPort)
    {
        _serialPort->println("WebSocket client disconnected");
    }
}

/**
 * @brief Handle WiFi events (static).
 * @param event WiFi event ID
 */
void SmartServoBridge::_wifiEvent(arduino_event_id_t event)
{
    if (!_instance || !_instance->_serialPort)
        return;

    // Only log state changes
    static arduino_event_id_t lastEvent = ARDUINO_EVENT_MAX;
    if (event == lastEvent)
        return;
    lastEvent = event;

    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_START:
        _instance->_serialPort->println("WiFi client started");
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        _instance->_serialPort->println("WiFi client stopped");
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        _instance->_serialPort->println("Connected to WiFi");
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        _instance->_serialPort->println("Disconnected from WiFi");
        _instance->_wifiConnected = false;
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        _instance->_serialPort->print("Got IP: ");
        _instance->_serialPort->println(WiFi.localIP());
        _instance->_wifiConnected = true;
        break;
    }
}