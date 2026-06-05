import paho.mqtt.client as mqtt
import json
import ssl
import time

# HiveMQ Cloud settings
BROKER = "220b2344066a4e288a4babcc5d307788.s1.eu.hivemq.cloud"
PORT = 8883
USERNAME = "espuser"
PASSWORD = "Espmodule@32"
TOPIC = "ota/firmware/update"
CA_CERT = "server/certs/hivemq_ca.pem"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to HiveMQ broker")
    else:
        print(f"Connection failed with code {rc}")

def on_publish(client, userdata, mid):
    print(f"Message published (ID: {mid})")

def publish_firmware(firmware_path, manifest_path):
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.on_connect = on_connect
    client.on_publish = on_publish
    
    # TLS configuration
    client.tls_set(ca_certs=CA_CERT, tls_version=ssl.PROTOCOL_TLS)
    client.username_pw_set(USERNAME, PASSWORD)
    
    # Connect
    print(f"Connecting to {BROKER}:{PORT}...")
    client.connect(BROKER, PORT, keepalive=60)
    client.loop_start()
    
    # Read files
    with open(firmware_path, "rb") as f:
        firmware_data = f.read()
    with open(manifest_path, "r") as f:
        manifest = json.load(f)
    
    # Create payload
    payload = {
        "manifest": manifest,
        "firmware": firmware_data.hex()
    }
    
    # Publish
    print(f"Publishing to topic: {TOPIC}")
    result = client.publish(TOPIC, json.dumps(payload), qos=1)
    
    # Wait for publish to complete
    time.sleep(2)
    client.loop_stop()
    client.disconnect()
    print("Disconnected")

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print("Usage: python mqtt_publish.py <firmware.bin> <manifest.json>")
        sys.exit(1)
    
    publish_firmware(sys.argv[1], sys.argv[2])