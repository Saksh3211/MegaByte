import time
from crypto.hashing import hash_block
from core.merkle import merkle_root

class Block:
    def __init__(self, index, transactions, previous_hash, difficulty):
        self.index = index
        self.timestamp = time.time()
        self.transactions = transactions
        self.previous_hash = previous_hash
        self.nonce = 0
        self.difficulty = difficulty
        self.hash = None
        self.merkle_root = merkle_root(transactions)

    def compute_hash(self):
        return hash_block(self.__dict__)