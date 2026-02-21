import json
import os
import hashlib
from wallet import Wallet

WALLET_FILE = "Frontend/wallets.json"


class WalletManager:
    def __init__(self):
        self.wallets = self.load_wallets()
        self.current_wallet = None

    # -------------------------
    # Persistence
    # -------------------------

    def load_wallets(self):
        if not os.path.exists(WALLET_FILE):
            return {}
        with open(WALLET_FILE, "r") as f:
            return json.load(f)

    def save_wallets(self):
        with open(WALLET_FILE, "w") as f:
            json.dump(self.wallets, f, indent=4)

    # -------------------------
    # Password Hashing
    # -------------------------

    def hash_password(self, password):
        return hashlib.sha256(password.encode()).hexdigest()

    # -------------------------
    # Create Wallet
    # -------------------------

    def create_wallet(self, username, password):
        if username in self.wallets:
            return {"error": "Username already exists"}

        wallet = Wallet()
        wallet.create_keys()

        self.wallets[username] = {
            "password": self.hash_password(password),
            "address": wallet.address,
            "private_key": wallet.private_key,
            "public_key": wallet.public_key
        }

        self.save_wallets()
        self.current_wallet = self.wallets[username]

        return {"status": "Wallet created", "address": wallet.address}

    # -------------------------
    # Login Wallet
    # -------------------------

    def login(self, username, password):
        if username not in self.wallets:
            return {"error": "Wallet not found"}

        if self.wallets[username]["password"] != self.hash_password(password):
            return {"error": "Invalid password"}

        self.current_wallet = self.wallets[username]
        return {"status": "Logged in", "address": self.current_wallet["address"]}

    def logout(self):
        self.current_wallet = None