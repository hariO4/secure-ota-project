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
        print("Connected to HiveMQ broker")
    else:
        print(f"Connection failed with code {rc}")

def on_publish(client, userdata, mid):
    print(f"Message published (ID: {mid})")

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
    
    client.tls_set(ca_certs=CA_CERT, tls_version=ssl.PROTOCOL_TLS)
    client.username_pw_set(USERNAME, PASSWORD)

    print(f"Connecting to {BROKER}:{PORT}...")
    client.connect(BROKER, PORT, keepalive=60)
    client.loop_start()
    time.sleep(2)

    # Publish firmware as