# Secure OTA Firmware Update System

![OTA Release](https://github.com/hari04/secure-ota-project/actions/workflows/ota_release.yml/badge.svg)

Production-grade secure over-the-air firmware update pipeline for ESP32 IoT devices. This repository handles server-side cryptographic signing, automated CI/CD release, and TLS-encrypted MQTT delivery.

## What This Does

- Compiles and signs firmware binaries using RSA-2048
- Automates build-sign-publish via GitHub Actions
- Delivers signed firmware over TLS-MQTT to subscribed devices
- Generates versioned manifests with SHA-256 hashes and signatures

## Architecture

```
Developer → GitHub → GitHub Actions → Sign → HiveMQ MQTT → ESP32 Device
```

## Repository Structure

```
secure-ota-project/
    server/
        keys/           # RSA key pair (private key excluded via .gitignore)
        scripts/        # Python signing, verification, manifest and MQTT scripts
        certs/          # HiveMQ TLS CA certificate
    device/
        main/
            main.c                  # App entry point and boot logic
            wifi_manager.c/h        # WiFi connection handler
            my_mqtt_client.c/h      # MQTT subscriber and OTA trigger
            crypto_verify.c/h       # RSA-2048 signature verification
            ota_handler.c/h         # OTA flash logic
            public_key_data.c/h     # Embedded RSA public key
            hivemq_ca.pem           # HiveMQ TLS certificate
            CMakeLists.txt          # Component build config
        partitions.csv              # Custom 4MB partition table
        CMakeLists.txt              # Project build config
        README.md                   # Device side documentation
    .github/
        workflows/      # CI/CD automation
    firmware/           # Firmware binaries for release
    README.md           # Main project documentation
```

## Scripts

| Script | Purpose |
|--------|---------|
| `server/scripts/sign_firmware.py` | Signs firmware binary with RSA private key |
| `server/scripts/verify_signature.py` | Local verification using public key |
| `server/scripts/create_manifest.py` | Generates JSON manifest with hash and signature |
| `server/scripts/mqtt_publish.py` | Publishes signed firmware to HiveMQ over TLS |

## Team

- **Harikrishnan P** — Server-side signing, CI/CD, MQTT broker
- **Mohamed Ayaan** — ESP32 firmware, mbedTLS verification, OTA partitions

## Security

- Private key stored in GitHub Secrets, never committed
- All MQTT traffic encrypted via TLS 1.2
- Firmware rejected if signature verification fails
- Automatic rollback on boot failure (device-side)

## License

MIT
