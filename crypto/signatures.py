from ecdsa import VerifyingKey, SECP256k1

def verify_signature(public_key_hex, message, signature_hex):
    vk = VerifyingKey.from_string(bytes.fromhex(public_key_hex), curve=SECP256k1)
    return vk.verify(bytes.fromhex(signature_hex), message.encode())
