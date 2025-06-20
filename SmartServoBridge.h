/*
 * SmartServoBridge.h
 *
 * A library for ESP32 that bridges between smart servos (like Dynamixel) and web clients.
 * Allows control and monitoring of smart servos over a network connection.
 *
 * Written by Nicholas Stedman (nick@robotstack.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 */

#ifndef SmartServoBridge_h
#define SmartServoBridge_h

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

/**
 * @brief SmartServoBridge bridges smart servos and web clients over WiFi using WebSockets or Serial.
 */
class SmartServoBridge
{
public:
    /**
     * @brief Construct a new SmartServoBridge object
     * @param servoBaudRate Baud rate for the servo bus (default: 1000000)
     * @param txEnablePin Pin to enable TX for half-duplex (default: D3)
     * @param rxPin RX pin for the servo bus (default: -1, use default)
     * @param txPin TX pin for the servo bus (default: -1, use default)
     */
    SmartServoBridge(uint32_t servoBaudRate = 1000000,
                     uint8_t txEnablePin = D3,
                     int8_t rxPin = -1,
                     int8_t txPin = -1);

    /**
     * @brief Set the serial port for debug output and/or serial relay mode
     * @param port Pointer to a Stream object (e.g., &Serial)
     */
    void setSerialPort(Stream *port);

    /**
     * @brief Initialize the library in WebSocket mode
     * @param ssid WiFi SSID
     * @param password WiFi password
     */
    void begin(const char *ssid, const char *password);

    /**
     * @brief Initialize the library in Serial mode
     * @param bridgePort Stream for bridge communication
     */
    void begin(Stream *bridgePort);

    /**
     * @brief Main loop function. Call this frequently in your main loop.
     */
    void update();

    /**
     * @brief Set a callback for when a command is sent to the servo.
     * @param handler Function pointer to the callback
     */
    void setServoCommandHandler(void (*handler)(uint8_t *, size_t));

    /**
     * @brief Set a callback for when a response is received from the servo.
     * @param handler Function pointer to the callback
     */
    void setServoResponseHandler(void (*handler)(uint8_t *, size_t));

    /**
     * @brief Set a callback for when a text message is received over WebSocket.
     * @param handler Function pointer to the callback
     */
    void setTextMessageHandler(void (*handler)(const char *));

    /**
     * @brief Check if WiFi is connected (WebSocket mode only).
     * @return true if connected, false otherwise
     */
    bool isConnected();

    /**
     * @brief Get the local IP address (WebSocket mode only).
     * @return IPAddress object
     */
    IPAddress getLocalIP();

    /**
     * @brief Get the current WiFi RSSI (signal strength) (WebSocket mode only).
     * @return RSSI in dBm
     */
    int getRSSI();

    /**
     * @brief Enable debug output on the specified port
     * @param debugPort Stream for debug output
     * @param enable true to enable, false to disable (default: true)
     */
    void enableDebug(Stream *debugPort, bool enable = true);

    /**
     * @brief Disable debug output
     */
    void disableDebug();

    /**
     * @brief Check if debug output is currently available
     * @return true if debug output is safe to use
     */
    bool isDebugAvailable();

    /**
     * @brief Relay data to the servo bus.
     * @param mem Pointer to data
     * @param len Length of data
     */
    void relayToServo(const void *mem, uint32_t len);

    /**
     * @brief Send a text message over WebSocket to connected clients.
     * @param message Text message to send
     */
    void sendTextMessage(const String &message);

private:
    /**
     * @brief Pin used to enable TX for half-duplex communication.
     */
    uint8_t _txEnablePin;

    /**
     * @brief Baud rate for the servo bus.
     */
    uint32_t _servoBaudRate;

    /**
     * @brief RX pin for the servo bus.
     */
    int8_t _rxPin;

    /**
     * @brief TX pin for the servo bus.
     */
    int8_t _txPin;

    /**
     * @brief Wait time (in microseconds) after flushing the servo bus.
     */
    uint16_t _waitForFlushComplete = 10;

    /**
     * @brief WiFi SSID.
     */
    const char *_ssid = nullptr;

    /**
     * @brief WiFi password.
     */
    const char *_password = nullptr;

    /**
     * @brief HardwareSerial object for the servo bus.
     */
    HardwareSerial _servoBus{0};

    /**
     * @brief WebSocketsServer object for WebSocket communication.
     */
    WebSocketsServer _webSocket{8080};

    /**
     * @brief WiFi connection state.
     */
    bool _wifiConnected = false;

    /**
     * @brief Serial port for debug output and/or serial relay.
     */
    Stream *_serialPort = nullptr;

    /**
     * @brief Flag to indicate if using serial instead of websocket.
     */
    bool _useSerial = false;

    /**
     * @brief Track TX_EN state.
     */
    bool _txEnabled = false;

    /**
     * @brief Track if begin() has been called.
     */
    bool _isInitialized = false;

    /**
     * @brief Debug output enabled flag.
     */
    bool _debugEnabled = false;

    /**
     * @brief Callback for servo commands.
     */
    void (*_servoCommandHandler)(uint8_t *, size_t) = nullptr;

    /**
     * @brief Callback for servo responses.
     */
    void (*_servoResponseHandler)(uint8_t *, size_t) = nullptr;

    /**
     * @brief Callback for text messages.
     */
    void (*_textMessageHandler)(const char *) = nullptr;

    /**
     * @brief Static instance for WiFi event handling.
     */
    static SmartServoBridge *_instance;

    /**
     * @brief Setup WiFi and WebSocket networking.
     */
    void _setupNetworking();

    /**
     * @brief Relay data to the servo bus.
     * @param mem Pointer to data
     * @param len Length of data
     */
    void _relayToServo(const void *mem, uint32_t len);

    /**
     * @brief Relay data from the servo bus.
     */
    void _relayFromServo();

    /**
     * @brief Handle WebSocket events.
     * @param num Client number
     * @param type Event type
     * @param payload Data payload
     * @param length Length of payload
     */
    void _webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);

    /**
     * @brief Handle WiFi events (static).
     * @param event WiFi event ID
     */
    static void _wifiEvent(arduino_event_id_t event);

    /**
     * @brief Handle serial data in serial mode.
     */
    void _handleSerialData();

    /**
     * @brief Stream for bridge communication (in Serial mode)
     */
    Stream *_bridgePort = nullptr;

    /**
     * @brief Stream for debug output
     */
    Stream *_debugPort = nullptr;
};

#endif