# Device Firmware —(ESP32)

## Overview
ESP32 device-side firmware for the Secure OTA Firmware Update System.
Handles WiFi connectivity, MQTT subscription, RSA-2048 signature 
verification, OTA flashing, and automatic rollback protection.

## Hardware Required
- ESP32-WROOM-32 DevKit V1 (CP2102)
- USB-C to Micro-USB data cable
- Wi-Fi router (2.4 GHz)

## Software Required
- ESP-IDF v6.0.1
- VS Code + ESP-IDF Extension

## Project File Structure

| File | Location | Description |
|---|---|---|
| `main.c` | `device/main/` | App entry point and boot logic |
| `wifi_manager.c` | `device/main/` | WiFi connection handler |
| `wifi_manager.h` | `device/main/` | WiFi header file |
| `my_mqtt_client.c` | `device/main/` | MQTT subscriber and OTA trigger |
| `my_mqtt_client.h` | `device/main/` | MQTT header file |
| `crypto_verify.c` | `device/main/` | RSA-2048 signature verification |
| `crypto_verify.h` | `device/main/` | Crypto header file |
| `ota_handler.c` | `device/main/` | OTA flash logic |
| `ota_handler.h` | `device/main/` | OTA header file |
| `public_key_data.c` | `device/main/` | Embedded RSA public key |
| `public_key_data.h` | `device/main/` | Public key header file |
| `hivemq_ca.pem` | `device/main/` | HiveMQ TLS certificate |
| `CMakeLists.txt` | `device/main/` | Component build config |
| `partitions.csv` | `device/` | Custom 4MB partition table |
| `CMakeLists.txt` | `device/` | Project build config |
| `README.md` | `device/` | This file |

## How to Build and Flash
```bash
# Set up ESP-IDF environment
# Windows: Open IDF_v6.0.1_Powershell from Start Menu

# Build
idf.py build

# Flash and monitor
idf.py -p COM10 flash monitor
```

## How It Works

### 1. Boot Sequence
- ESP32 boots and shows current running partition (ota_0 or ota_1)
- Connects to WiFi automatically
- Marks app as valid (cancels rollback timer)
- Connects to HiveMQ MQTT broker over TLS

### 2. OTA Update Flow
- Subscribes to two MQTT topics:
  - `ota/firmware/binary` — receives firmware chunks
  - `ota/firmware/signature` — receives RSA signature
- Firmware chunks written directly to flash as received
- After all chunks received — signature verified from flash
- If valid: sets boot partition and reboots
- If invalid: rejects update, continues running

### 3. Security Features
- RSA-2048 asymmetric key verification
- SHA-256 hash computed from flash using streaming (no large RAM buffer)
- Signature verified before any partition switch
- TLS-encrypted MQTT channel

### 4. Rollback Protection
- ESP-IDF rollback enabled in bootloader config
- App must call esp_ota_mark_app_valid_cancel_rollback() after successful boot
- If new firmware crashes before confirmation — automatic rollback to previous partition

## Partition Table
| Name    | Type | Size   | Purpose              |
|---------|------|--------|----------------------|
| nvs     | data | 20 KB  | Non-volatile storage |
| otadata | data | 8 KB   | OTA state tracking   |
| ota_0   | app  | 1.75MB | Primary app slot     |
| ota_1   | app  | 1.75MB | OTA update slot      |

## MQTT Topics
| Topic                    | Direction | Content          |
|--------------------------|-----------|------------------|
| ota/firmware/binary      | Subscribe | Raw firmware binary |
| ota/firmware/signature   | Subscribe | RSA-2048 signature  |

## Member Responsibilities
- **Harikrishnan P —** Server-side signing, CI/CD, MQTT broker
- **Mohamed Ayaan —** ESP32 firmware, mbedTLS verification, OTA partitions

