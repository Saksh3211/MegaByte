import hashlib
import secrets


class Wallet:

    def __init__(self):
        self.private_key = None
        self.public_key = None
        self.address = None

    def create_keys(self):
        self.private_key = secrets.token_hex(32)
        self.public_key = hashlib.sha256(self.private_key.encode()).hexdigest()
        self.address = hashlib.sha256(self.public_key.encode()).hexdigest()