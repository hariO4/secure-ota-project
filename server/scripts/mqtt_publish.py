import paho.mqtt.client as mqtt
import ssl
import time

BROKER = "220b2344066a4e288a4babcc5d307788.s1.eu.hivemq.cloud"
PORT = 8883
USERNAME = "espuser"
PASSWORD = "Espmodule@32"
CA_CERT = "server/certs/hivemq_ca.pem"

TOPIC_FIRMWARE = "ota/firmware/binary"
TOPIC_SIGNATURE = "ota/firmware/signature"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("✓ Connected to HiveMQ broker")
    else:
        print(f"✗ Connection failed with code {rc}")

def on_publish(client, userdata, mid):
    print(f"✓ Message published (ID: {mid})")

def on_log(client, userdata, level, buf):
    print(f"MQTT Log: {buf}")

def publish_ota(firmware_file, signature_file):
    with open(firmware_file, "rb") as f:
        firmware_data = f.read()
    with open(signature_file, "rb") as f:
        signature_data = f.read()

    print(f"Firmware size: {len(firmware_data)} bytes")
    print(f"Signature size: {len(signature_data)} bytes")

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.on_connect = on_connect
    client.on_publish = on_publish
    client.on_log = on_log
    
    client.tls_set(ca_certs=CA_CERT, tls_version=ssl.PROTOCOL_TLS)
    client.username_pw_set(USERNAME, PASSWORD)

    print(f"Connecting to {BROKER}:{PORT}...")
    
    try:
        client.connect(BROKER, PORT, keepalive=60)
        client.loop_start()
        
        # Wait for connection with timeout
        timeout = 10
        start_time = time.time()
        while not client.is_connected() and time.time() - start_time < timeout:
            time.sleep(0.5)
        
        if not client.is_connected():
            print("✗ Failed to connect within timeout")
            client.loop_stop()
            return
        
        print("Publishing firmware...")
        result1 = client.publish(TOPIC_FIRMWARE, firmware_data, qos=1)
        time.sleep(2)
        
        print("Publishing signature...")
        result2 = client.publish(TOPIC_SIGNATURE, signature_data, qos=1)
        time.sleep(2)
        
        client.loop_stop()
        client.disconnect()
        print("Done! OTA update sent.")
        
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print("Usage: python mqtt_publish.py <firmware.bin> <signature.sig>")
        sys.exit(1)
    publish_ota(sys.argv[1], sys.argv[2])