# Key Management

## Files
- `private_key.pem` — SECRET private key. Never share. Never commit.
- `public_key.pem` — Public key. Shared with Member B for ESP32 embedding.

## What if private key is lost?
You cannot sign new firmware updates. The device will reject any firmware signed with a new key.
Keep backups securely (password manager, encrypted USB, etc.).