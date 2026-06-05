import json
import hashlib
import base64
import sys
from datetime import datetime

def create_manifest(firmware_path, sig_path, version):
    # Read firmware
    with open(firmware_path, "rb") as f:
        firmware_data = f.read()
    
    # Read signature
    with open(sig_path, "rb") as f:
        signature = f.read()
    
    # Create manifest
    manifest = {
        "version": version,
        "filename": firmware_path.split("/")[-1].split("\\")[-1],
        "sha256": hashlib.sha256(firmware_data).hexdigest(),
        "signature": base64.b64encode(signature).decode('utf-8'),
        "timestamp": datetime.now().isoformat(),
        "size": len(firmware_data)
    }
    
    # Save manifest
    output_path = firmware_path.replace(".bin", "_manifest.json")
    with open(output_path, "w") as f:
        json.dump(manifest, f, indent=2)
    
    print(f"✓ Manifest saved to: {output_path}")
    print(json.dumps(manifest, indent=2))
    return output_path

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python create_manifest.py <firmware.bin> <signature.sig> <version>")
        sys.exit(1)
    
    create_manifest(sys.argv[1], sys.argv[2], sys.argv[3])