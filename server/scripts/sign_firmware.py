import sys
import hashlib
from pathlib import Path
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding

def sign_firmware(firmware_path, private_key_path, output_sig_path):
    # Read firmware binary
    with open(firmware_path, "rb") as f:
        firmware_data = f.read()
    
    # Compute SHA-256 hash
    firmware_hash = hashlib.sha256(firmware_data).digest()
    
    # Load private key
    with open(private_key_path, "rb") as f:
        private_key = serialization.load_pem_private_key(f.read(), password=None)
    
    # Sign the hash with RSA-PKCS1v15
    signature = private_key.sign(
        firmware_hash,
        padding.PKCS1v15(),
        hashes.SHA256()
    )
    
    # Save signature to file
    with open(output_sig_path, "wb") as f:
        f.write(signature)
    
    print(f"✓ Signed: {firmware_path}")
    print(f"✓ Signature saved to: {output_sig_path}")
    print(f"✓ Firmware size: {len(firmware_data)} bytes")
    print(f"✓ Signature size: {len(signature)} bytes")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python sign_firmware.py <firmware.bin> <private_key.pem> <output.sig>")
        sys.exit(1)
    
    sign_firmware(sys.argv[1], sys.argv[2], sys.argv[3])