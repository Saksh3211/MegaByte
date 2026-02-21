from ecdsa import SigningKey, SECP256k1
import hashlib

def generate_keys():
    private_key = SigningKey.generate(curve=SECP256k1)
    public_key = private_key.get_verifying_key()

    pub_bytes = public_key.to_string()
    sha = hashlib.sha256(pub_bytes).hexdigest()

    return {
        "private_key": private_key.to_string().hex(),
        "public_key": pub_bytes.hex(),
        "address": sha[:40]
    }