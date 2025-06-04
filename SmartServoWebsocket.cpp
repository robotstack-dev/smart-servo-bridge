#include "SmartServoWebsocket.h"

// Initialize static member
SmartServoWebsocket *SmartServoWebsocket::_instance = nullptr;

// Constants
#define WIFI_CONNECT_TIMEOUT 20000 // 20 seconds timeout
#define WIFI_RECONNECT_DELAY 5000  // 5 seconds between reconnect attempts
#define MIN_WIFI_RSSI -80          // More lenient signal strength threshold

/**
 * @brief Construct a new SmartServoWebsocket object.
 * @param websocketPort The port to use for the WebSocket server (default: 80)
 */
SmartServoWebsocket::SmartServoWebsocket(uint16_t websocketPort)
    : _webSocket(websocketPort)
{
    _instance = this;
}

/**
 * @brief Initialize the library and connect to WiFi and the servo bus.
 * @param ssid WiFi SSID
 * @param password WiFi password
 * @param servoBaudRate Baud rate for the servo bus (default: 1000000)
 * @param rxPin RX pin for the servo bus (default: -1, use default)
 * @param txPin TX pin for the servo bus (default: -1, use default)
 * @param txEnablePin Pin to enable TX for half-duplex (default: D3)
 */
void SmartServoWebsocket::begin(const char *ssid, const char *password, uint32_t servoBaudRate, int8_t rxPin, int8_t txPin, uint8_t txEnablePin)
{
    _ssid = ssid;
    _password = password;
    _servoBaudRate = servoBaudRate;
    _txEnablePin = txEnablePin;

    // Initialize pins
    pinMode(_txEnablePin, OUTPUT);
    digitalWrite(_txEnablePin, LOW);

    // Initialize serial communication
    if (_debugStream)
    {
        _debugStream->print("Initializing serial port at ");
        _debugStream->print(_servoBaudRate);
        _debugStream->print(", RX: ");
        _debugStream->print(rxPin);
        _debugStream->print(", TX: ");
        _debugStream->print(txPin);
        _debugStream->print(", TX_EN: ");
        _debugStream->print(_txEnablePin);
        _debugStream->println(")");
    }
    _servoBus.begin(_servoBaudRate, SERIAL_8N1, rxPin, txPin);

    // Setup networking
    _setupNetworking();
}

/**
 * @brief Set the debug output stream (e.g., Serial).
 * @param stream Reference to a Stream object for debug output
 */
void SmartServoWebsocket::setDebugStream(Stream &stream)
{
    _debugStream = &stream;
}

/**
 * @brief Main loop function. Call this frequently in your main loop.
 */
void SmartServoWebsocket::update()
{
    if (_wifiConnected)
    {
        // Check if we still have a good connection
        if (WiFi.status() != WL_CONNECTED)
        {
            if (_debugStream)
            {
                _debugStream->println("Connection lost");
            }
            _wifiConnected = false;
        }
        else if (WiFi.RSSI() < MIN_WIFI_RSSI)
        {
            if (_debugStream)
            {
                _debugStream->print("Warning: Weak signal (RSSI: ");
                _debugStream->print(WiFi.RSSI());
                _debugStream->println(" dBm)");
            }
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
 * @brief Set a callback for when a command is sent to the servo.
 * @param handler Function pointer to the callback
 */
void SmartServoWebsocket::setServoCommandHandler(void (*handler)(uint8_t *, size_t))
{
    _servoCommandHandler = handler;
}

/**
 * @brief Set a callback for when a response is received from the servo.
 * @param handler Function pointer to the callback
 */
void SmartServoWebsocket::setServoResponseHandler(void (*handler)(uint8_t *, size_t))
{
    _servoResponseHandler = handler;
}

/**
 * @brief Set a callback for when a text message is received over WebSocket.
 * @param handler Function pointer to the callback
 */
void SmartServoWebsocket::setTextMessageHandler(void (*handler)(const char *))
{
    _textMessageHandler = handler;
}

/**
 * @brief Check if WiFi is connected.
 * @return true if connected, false otherwise
 */
bool SmartServoWebsocket::isConnected()
{
    return _wifiConnected;
}

/**
 * @brief Get the local IP address.
 * @return IPAddress object
 */
IPAddress SmartServoWebsocket::getLocalIP()
{
    return WiFi.localIP();
}

/**
 * @brief Get the current WiFi RSSI (signal strength).
 * @return RSSI in dBm
 */
int SmartServoWebsocket::getRSSI()
{
    return WiFi.RSSI();
}

/**
 * @brief Setup WiFi and WebSocket networking.
 */
void SmartServoWebsocket::_setupNetworking()
{
    if (_debugStream)
    {
        _debugStream->println();
        _debugStream->print("Attaching to WiFi '");
        _debugStream->print(_ssid);
        _debugStream->println("'...");
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
            if (_debugStream)
                _debugStream->print('.');
            delay(500);
        }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        int rssi = WiFi.RSSI();
        if (rssi < MIN_WIFI_RSSI && _debugStream)
        {
            _debugStream->print("\nWarning: Weak WiFi signal (RSSI: ");
            _debugStream->print(rssi);
            _debugStream->println(" dBm)");
        }

        _wifiConnected = true;
        if (_debugStream)
        {
            _debugStream->print("Network connected.\nLocal IP address: ");
            _debugStream->println(WiFi.localIP());
            _debugStream->println("Ready! Use port 80 to connect.");
        }

        _webSocket.begin();
        _webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t *payload, size_t length)
                           { this->_webSocketEvent(num, type, payload, length); });
    }
    else
    {
        if (_debugStream)
        {
            _debugStream->println("\nFailed to connect to WiFi!");
            _debugStream->print("Status: ");
            _debugStream->println(WiFi.status());
        }
        _wifiConnected = false;
    }
}

/**
 * @brief Relay data from WebSocket to the servo bus.
 * @param mem Pointer to data
 * @param len Length of data
 */
void SmartServoWebsocket::_relayToServo(const void *mem, uint32_t len)
{
    const uint8_t *src = (const uint8_t *)mem;

    if (_debugStream)
    {
        _debugStream->print("Sending to servo (");
        _debugStream->print(len);
        _debugStream->println(" bytes):");
        for (uint32_t i = 0; i < len; i++)
        {
            _debugStream->printf("%02X ", src[i]);
        }
        _debugStream->println();
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
 * @brief Relay data from the servo bus to WebSocket clients.
 */
void SmartServoWebsocket::_relayFromServo()
{
    while (_servoBus.available())
    {
        long len = _servoBus.available();
        uint8_t buffer[len];
        _servoBus.readBytes(buffer, len);

        if (_debugStream)
        {
            _debugStream->println("\nReply from servo:");
            for (int i = 0; i < len; i++)
            {
                _debugStream->printf("%02X ", buffer[i]);
            }
            _debugStream->println();
        }

        _webSocket.sendBIN(0, buffer, len);

        if (_servoResponseHandler)
        {
            _servoResponseHandler(buffer, len);
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
void SmartServoWebsocket::_webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_DISCONNECTED:
        if (_debugStream)
        {
            _debugStream->printf("[%u] Disconnected!\n", num);
        }
        break;

    case WStype_CONNECTED:
        if (_debugStream)
        {
            IPAddress ip = _webSocket.remoteIP(num);
            _debugStream->printf("[%u] Connected from %d.%d.%d.%d\n",
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

            if (_debugStream)
            {
                _debugStream->printf("[%u] Received text: %s\n", num, text);
            }

            _textMessageHandler(text);
            delete[] text;
        }
        break;

    case WStype_BIN:
        _relayToServo(payload, length);
        if (_debugStream)
        {
            _debugStream->printf("[%u] Received binary length: %u\n", num, length);
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
void SmartServoWebsocket::_wifiEvent(arduino_event_id_t event)
{
    if (!_instance || !_instance->_debugStream)
        return;

    // Only log state changes
    static arduino_event_id_t lastEvent = ARDUINO_EVENT_MAX;
    if (event == lastEvent)
        return;
    lastEvent = event;

    switch (event)
    {
    case ARDUINO_EVENT_WIFI_STA_START:
        _instance->_debugStream->println("WiFi client started");
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        _instance->_debugStream->println("WiFi client stopped");
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        _instance->_debugStream->println("Connected to WiFi");
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        _instance->_debugStream->println("Disconnected from WiFi");
        _instance->_wifiConnected = false;
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        _instance->_debugStream->print("Got IP: ");
        _instance->_debugStream->println(WiFi.localIP());
        _instance->_wifiConnected = true;
        break;
    }
}