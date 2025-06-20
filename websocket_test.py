#!/usr/bin/env python3
"""
Simple WebSocket test client for SmartServoBridge
Usage: python3 websocket_test.py <esp32_ip_address>
"""

import sys
import asyncio
import websockets
import json
import time

async def test_websocket(ip_address):
    """Test WebSocket connection to ESP32"""
    uri = f"ws://{ip_address}:8080"
    
    print(f"Attempting to connect to {uri}")
    
    try:
        async with websockets.connect(uri) as websocket:
            print("✅ WebSocket connection established!")
            
            # Send a test message
            test_message = "Hello ESP32!"
            print(f"Sending: {test_message}")
            await websocket.send(test_message)
            
            # Wait for response
            try:
                response = await asyncio.wait_for(websocket.recv(), timeout=5.0)
                print(f"Received: {response}")
            except asyncio.TimeoutError:
                print("⚠️  No response received within 5 seconds")
            
            # Send a ping
            print("Sending ping...")
            await websocket.send("ping")
            
            try:
                response = await asyncio.wait_for(websocket.recv(), timeout=5.0)
                print(f"Ping response: {response}")
            except asyncio.TimeoutError:
                print("⚠️  No ping response received")
            
            # Keep connection alive for a few seconds
            print("Keeping connection alive for 5 seconds...")
            await asyncio.sleep(5)
            
    except websockets.exceptions.InvalidURI:
        print(f"❌ Invalid URI: {uri}")
        return False
    except websockets.exceptions.ConnectionClosed:
        print("❌ Connection closed unexpectedly")
        return False
    except websockets.exceptions.InvalidStatusCode as e:
        print(f"❌ Invalid status code: {e}")
        return False
    except ConnectionRefusedError:
        print(f"❌ Connection refused to {uri}")
        print("   Make sure the ESP32 is running and the WebSocket server is started")
        return False
    except Exception as e:
        print(f"❌ Connection failed: {e}")
        return False
    
    return True

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 websocket_test.py <esp32_ip_address>")
        print("Example: python3 websocket_test.py 192.168.2.61")
        sys.exit(1)
    
    ip_address = sys.argv[1]
    
    print("SmartServoBridge WebSocket Test Client")
    print("=" * 40)
    
    # Test connection
    success = asyncio.run(test_websocket(ip_address))
    
    if success:
        print("\n✅ WebSocket test completed successfully!")
    else:
        print("\n❌ WebSocket test failed!")
        print("\nTroubleshooting tips:")
        print("1. Make sure the ESP32 is powered on and connected to WiFi")
        print("2. Verify the IP address is correct")
        print("3. Check that the WebSocket server is running on port 8080")
        print("4. Ensure your computer and ESP32 are on the same network")
        print("5. Check for any firewall blocking port 8080")

if __name__ == "__main__":
    main() 