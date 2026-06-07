from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding
from cryptography.hazmat.backends import default_backend
import hashlib

# Load private key
with open("server/keys/private_key.pem", "rb") as f:
    private_key = serialization.load_pem_private_key(
        f.read(), password=None, backend=default_backend()
    )

# Load public key
with open("server/keys/public_key.pem", "rb") as f:
    public_key = serialization.load_pem_public_key(
        f.read(), backend=default_backend()
    )

# Load firmware
with open("firmware/secure_ota_device.bin", "rb") as f:
    firmware = f.read()

# Load signature
with open("firmware/secure_ota_device.sig", "rb") as f:
    signature = f.read()

print(f"Firmware size: {len(firmware)} bytes")
print(f"Signature size: {len(signature)} bytes")

# Test 1 — Does private key match public key?
derived_public = private_key.public_key()
derived_pem = derived_public.public_bytes(
    encoding=serialization.Encoding.PEM,
    format=serialization.PublicFormat.SubjectPublicKeyInfo
)
with open("server/keys/public_key.pem", "rb") as f:
    original_pem = f.read()

if derived_pem == original_pem:
    print("Test 1: Keys MATCH")
else:
    print("Test 1: Keys DO NOT MATCH")

# Test 2 — Does signature verify with public key?
try:
    public_key.verify(
        signature,
        firmware,
        padding.PKCS1v15(),
        hashes.SHA256()
    )
    print("Test 2: Signature VALID on PC!")
except Exception as e:
    print(f"Test 2: Signature INVALID on PC: {e}")

# Test 3 — Print public key fingerprint
pub_der = public_key.public_bytes(
    encoding=serialization.Encoding.DER,
    format=serialization.PublicFormat.SubjectPublicKeyInfo
)
fingerprint = hashlib.sha256(pub_der).hexdigest()
print(f"Public key fingerprint: {fingerprint[:16]}...")
