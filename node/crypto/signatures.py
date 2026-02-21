from ecdsa import SigningKey, VerifyingKey, SECP256k1

def sign_transaction(private_key_hex, message):
    private_key = SigningKey.from_string(bytes.fromhex(private_key_hex), curve=SECP256k1)
    return private_key.sign(message.encode()).hex()

def verify_signature(public_key_hex, message, signature_hex):
    public_key = VerifyingKey.from_string(bytes.fromhex(public_key_hex), curve=SECP256k1)
    return public_key.verify(bytes.fromhex(signature_hex), message.encode())