import sys
import hashlib
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding

def verify_signature(firmware_path, sig_path, public_key_path):
    # Read files
    with open(firmware_path, "rb") as f:
        firmware_data = f.read()
    with open(sig_path, "rb") as f:
        signature = f.read()
    with open(public_key_path, "rb") as f:
        public_key = serialization.load_pem_public_key(f.read())
    
    # Compute hash
    firmware_hash = hashlib.sha256(firmware_data).digest()
    
    # Verify
    try:
        public_key.verify(
            signature,
            firmware_hash,
            padding.PKCS1v15(),
            hashes.SHA256()
        )
        print("✓ SIGNATURE VALID")
        return True
    except Exception as e:
        print("✗ SIGNATURE INVALID")
        print(f"  Error: {e}")
        return False

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python verify_signature.py <firmware.bin> <signature.sig> <public_key.pem>")
        sys.exit(1)
    verify_signature(sys.argv[1], sys.argv[2], sys.argv[3])