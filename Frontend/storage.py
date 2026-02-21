from cryptography.fernet import Fernet
import base64
import hashlib
import json

def derive_key(password):
    return base64.urlsafe_b64encode(
        hashlib.sha256(password.encode()).digest()
    )

def save_wallet(wallet_data, password):
    key = derive_key(password)
    f = Fernet(key)
    encrypted = f.encrypt(json.dumps(wallet_data).encode())

    with open("Frontend/wallet.dat", "wb") as file:
        file.write(encrypted)

def load_wallet(password):
    key = derive_key(password)
    f = Fernet(key)

    with open("Frontend/wallet.dat", "rb") as file:
        encrypted = file.read()

    decrypted = f.decrypt(encrypted)
    return json.loads(decrypted.decode())