# Smart Servo Bridge

A WebSocket and Serial bridge for ESP32 that connects smart servos (like Dynamixel) to web or serial clients. Control and monitor smart servos over a network connection or directly via USB/serial.

> **Note:** This is an early version of the library. While it has been tested with several servo models, bugs may exist. Please report any issues you encounter.

## Features

- WebSocket server running on ESP32
- Serial port (USB/TTL) bridge for direct connection
- Direct binary communication with smart servos
- Support for text-based configuration commands (over WebSocket or Serial)
- Debug logging over Serial
- Tested with Dynamixel XL430-W250, FeeTech HLS3930M, WaveShare ST3215

## Hardware Requirements

- ESP32 development board (tested with Xiao ESP32-S3, QT Py ESP32-S3)
- Smart servo (tested with Dynamixel XL430-W250, FeeTech HLS3930M, WaveShare ST3215)
- Either:
  - Commercial: **[RobotStack Smart Servo Add-On Board](https://www.tindie.com/products/robotstack/smart-servo-addon-board/)** - A ready-to-use interface board
  - DIY: Half-duplex UART circuit (see wiring diagram)

## Wiring

The ESP32 needs to be connected to the smart servo using a half-duplex UART configuration. You can either:

1. Purchase a **[RobotStack Smart Servo Add-On Board](https://www.tindie.com/products/robotstack/smart-servo-addon-board/)** (recommended for reliable operation)
2. Build the Smart Servo Add-On Board, or similar according to the **[documentation](https://github.com/robotstack-dev/smart-servo-add-on)**

## Installation

1. Clone this repository
2. Place into Arduino > Libraries folder
3. Install required libraries:
   - WebSocketsServer (external)
   - WiFi (included with ESP32 core)
4. Open Example
5. Configure your WiFi credentials in the code (for WebSocket mode)
6. Upload to your ESP32

## Configuration

Edit these parameters in the code:

```cpp
const char* ssid = "yourSSID";
const char* password = "yourPassword";
#define TX_EN D3  // Change if using different pin
```

## Usage

- **WebSocket Mode:**
  - Call `bridge.begin(ssid, password);` to start in WebSocket mode.
  - Connect to the ESP32's IP address on port 8080 using a WebSocket client.
  - Send binary to control the servo or text for custom messages.
  - To identify your IP, run the program with debugging enabled (eg. bridge.enableDebug(&IO);)
- **Serial Mode:**
  - Call `bridge.begin(&Serial);` to start in Serial mode.
  - Connect to the ESP32 via USB/TTL serial.
  - Send binary or text directly over the serial port.

## Examples

- `examples/WebSocketBridge/WebSocketBridge.ino` – WebSocket mode
- `examples/SerialBridge/SerialBridge.ino` – Serial mode (raw relay)
- `examples/SerialTextCommands/SerialTextCommands.ino` – Serial mode (relay + text commands)
- `websocket_test.html` – WebSocket test client for debugging and testing connections

### Example Python Clients

- [Feetech Python SDK with WebSockets](https://github.com/robotstack-dev/ftservo-python-websockets)
- [Dynamixel Python SDK with WebSockets](https://github.com/robotstack-dev/dynamixel-python-sdk-websockets)

## Debugging

Enable debug logging output over Serial by uncommenting:

```cpp
// bridge.enableDebug(&IO); // Enable debug output
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License. See the LICENSE file for details.

## Author

Written by Nicholas Stedman (nick@robotstack.com)
