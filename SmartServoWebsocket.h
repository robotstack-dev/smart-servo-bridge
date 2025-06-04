/*
 * SmartServoWebsocket.h
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

#ifndef SmartServoWebsocket_h
#define SmartServoWebsocket_h

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

/**
 * @brief SmartServoWebsocket bridges smart servos and web clients over WiFi using WebSockets.
 */
class SmartServoWebsocket
{
public:
    /**
     * @brief Construct a new SmartServoWebsocket object
     * @param websocketPort The port to use for the WebSocket server (default: 80)
     */
    SmartServoWebsocket(uint16_t websocketPort = 80);

    /**
     * @brief Initialize the library and connect to WiFi and the servo bus.
     * @param ssid WiFi SSID
     * @param password WiFi password
     * @param servoBaudRate Baud rate for the servo bus (default: 1000000)
     * @param rxPin RX pin for the servo bus (default: -1, use default)
     * @param txPin TX pin for the servo bus (default: -1, use default)
     * @param txEnablePin Pin to enable TX for half-duplex (default: D3)
     */
    void begin(const char *ssid, const char *password, uint32_t servoBaudRate = 1000000, int8_t rxPin = -1, int8_t txPin = -1, uint8_t txEnablePin = D3);

    /**
     * @brief Set the debug output stream (e.g., Serial).
     * @param stream Reference to a Stream object for debug output
     */
    void setDebugStream(Stream &stream);

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
     * @brief Check if WiFi is connected.
     * @return true if connected, false otherwise
     */
    bool isConnected();

    /**
     * @brief Get the local IP address.
     * @return IPAddress object
     */
    IPAddress getLocalIP();

    /**
     * @brief Get the current WiFi RSSI (signal strength).
     * @return RSSI in dBm
     */
    int getRSSI();

private:
    /**
     * @brief Pin used to enable TX for half-duplex communication.
     */
    uint8_t _txEnablePin;

    /**
     * @brief Baud rate for the servo bus.
     */
    uint32_t _servoBaudRate = 1000000;

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
    WebSocketsServer _webSocket{80};

    /**
     * @brief WiFi connection state.
     */
    bool _wifiConnected = false;

    /**
     * @brief Debug output stream.
     */
    Stream *_debugStream = nullptr;

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
    static SmartServoWebsocket *_instance;

    /**
     * @brief Setup WiFi and WebSocket networking.
     */
    void _setupNetworking();

    /**
     * @brief Relay data from WebSocket to the servo bus.
     * @param mem Pointer to data
     * @param len Length of data
     */
    void _relayToServo(const void *mem, uint32_t len);

    /**
     * @brief Relay data from the servo bus to WebSocket clients.
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
};

#endif