# Smart Servo WebSocket

A WebSocket server for ESP32 that bridges between smart servos (like Dynamixel) and web clients. This allows you to control and monitor smart servos over a network connection.

## Features

- WebSocket server running on ESP32
- Direct binary communication with smart servos
- Support for text-based configuration commands
- Debug logging over Serial
- Status LED indication
- Tested with Dynamixel XL430-W250, FeeTech HLS3930M, WaveShare ST3215

## Hardware Requirements

- ESP32 development board (tested with Xiao ESP32-S3, QT Py ESP32-S3)
- Smart servo (tested with Dynamixel XL430-W250, FeeTech HLS3930M, WaveShare ST3215)
- Either:
  - DIY: Half-duplex UART circuit (see wiring diagram)
  - Commercial: **Smart Servo Add-On Board** - A ready-to-use interface board (see [hardware documentation](https://github.com/nsted/smart-servo-add-on))

## Wiring

The ESP32 needs to be connected to the smart servo using a half-duplex UART configuration. You can either:

1. Build your own circuit following this example: https://emanual.robotis.com/docs/en/dxl/x/xl430-w250/#communication-circuit
2. Use the **Smart Servo Add-On Board** (recommended for reliable operation)

For detailed information about the Add-On Board, including purchasing, installation, and features, please see the [hardware documentation](https://github.com/nsted/smart-servo-add-on).

## Installation

1. Clone this repository
2. Open the project in Arduino IDE
3. Install required libraries:
   - WebSocketsServer
   - WiFi
4. Configure your WiFi credentials in the code
5. Upload to your ESP32
6. Note your device's IP address in the Serial Monitor (Tools > Serial Monitor)

## Configuration

Edit these parameters in the code:

```cpp
const char* ssid = "yourSSID";
const char* password = "yourPassword";
#define TX_EN D8  // Change if using different pin
```

## Usage

1. Power on the ESP32
2. Connect to the same network as the ESP32
3. Connect to the WebSocket server at `ws://<esp32-ip>:80`
4. Send binary messages to control the servo
5. Receive binary responses from the servo

### Example Python Client

A Python client is available at: https://github.com/nsted/smart-servo-websocket-client

## Debugging

Enable debug output by uncommenting:

```cpp
#define DEBUG
```

Debug information will be sent over Serial at 1000000 baud.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

[Add your chosen license here]

## Author

Written by Nicholas Stedman (nick@robotstack.com)

## Related Projects

- [DynamixelSDK-websocket](https://github.com/nsted/DynamixelSDK-websocket) - Python client library
- [DynamixelSDK](https://github.com/ROBOTIS-GIT/DynamixelSDK) - Original Dynamixel SDK
