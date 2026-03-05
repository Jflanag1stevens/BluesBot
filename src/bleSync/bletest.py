import asyncio
from bleak import BleakScanner, BleakClient


WRITE_UUID = "0000dead-0000-1000-8000-00805f9b34fb"




async def main():
    # 1. Scan for BLE devices
    print("Scanning for BLE devices for 1 second...")
    devices = await BleakScanner.discover(timeout=1)

    esp32_address = None
    for d in devices:
        if d.name is None:
            continue
        if "BLE-Server" in d.name:  # match your ESP32 device name
            esp32_address = d.address
            print(f"Found ESP32: {d.name} ({d.address})")
            break

    if not esp32_address:
        print("ESP32 not found. Make sure it is advertising.")
        return

    # 2. Connect once and keep connection open
    client = BleakClient(esp32_address)
    try:
        print(f"Connecting to {esp32_address}...")
        await client.connect()
        print("Connected! Type 'exit' to quit.")

        # 3. Terminal input loop
        while True:
            cmd = input("> ").strip()
            if cmd.lower() == "exit":
                break
            else:
                """
                # Send ASCII string as bytes
                cmd = cmd.encode('utf-8').decode('unicode_escape')
                data_bytes = cmd.encode("ascii")
                await client.write_gatt_char(WRITE_UUID, data_bytes)
                print(f"Sent: {data_bytes}")
                """
                num = int(cmd)
                data_bytes = num.to_bytes(4,byteorder='little',signed=False)
                await client.write_gatt_char(WRITE_UUID, data_bytes)
                print(f"Sent: {data_bytes}")

    except Exception as e:
        print(f"Error: {e}")

    finally:
        await client.disconnect()
        print("Disconnected from ESP32.")

# Run the async main loop
asyncio.run(main())
