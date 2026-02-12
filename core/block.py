from crypto.hashing import sha256

class Block:
    def __init__(self, index, previous_hash, timestamp, transactions, nonce, difficulty, merkle_root):
        self.index = index
        self.previous_hash = previous_hash
        self.timestamp = timestamp
        self.transactions = transactions
        self.nonce = nonce
        self.difficulty = difficulty
        self.merkle_root = merkle_root
        self.hash = self.calculate_hash()

    def calculate_hash(self):
        data = f"{self.index}{self.previous_hash}{self.timestamp}{self.merkle_root}{self.nonce}{self.difficulty}"
        return sha256(data.encode())
