import paho.mqtt.client as mqtt
import ssl
import time
import base64

BROKER = "220b2344066a4e288a4babcc5d307788.s1.eu.hivemq.cloud"
PORT = 8883
USERNAME = "espuser"
PASSWORD = "Espmodule@32"
CA_CERT = "server/certs/hivemq_ca.pem"

TOPIC_FIRMWARE = "ota/firmware/binary"
TOPIC_SIGNATURE = "ota/firmware/signature"

def publish_ota(firmware_file, signature_file):
    with open(firmware_file, "rb") as f:
        firmware_data = f.read()
    with open(signature_file, "rb") as f:
        signature_data = f.read()

    print(f"Firmware size: {len(firmware_data)} bytes")
    print(f"Signature size: {len(signature_data)} bytes")

    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1)
    client.tls_set(ca_certs=CA_CERT, tls_version=ssl.PROTOCOL_TLS)
    client.username_pw_set(USERNAME, PASSWORD)

    print("Connecting to HiveMQ...")
    client.connect(BROKER, PORT, 60)
    client.loop_start()
    time.sleep(2)

    # Base64 encode to prevent MQTT binary corruption
    firmware_b64 = base64.b64encode(firmware_data).decode()
    signature_b64 = base64.b64encode(signature_data).decode()

    print(f"Publishing firmware (base64, {len(firmware_b64)} chars)...")
    client.publish(TOPIC_FIRMWARE, firmware_b64, qos=1)
    time.sleep(1)

    print(f"Publishing signature (base64, {len(signature_b64)} chars)...")
    client.publish(TOPIC_SIGNATURE, signature_b64, qos=1)
    time.sleep(1)

    client.loop_stop()
    client.disconnect()
    print("Done! OTA update sent.")

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 3:
        print("Usage: python mqtt_publish.py <firmware.bin> <signature.sig>")
        sys.exit(1)
    publish_ota(sys.argv[1], sys.argv[2])