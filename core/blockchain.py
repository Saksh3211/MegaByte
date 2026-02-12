import time
from consensus.pow import valid_proof
from core.difficulty import adjust_difficulty
from core.reward import calculate_reward
from config.genesis import create_genesis_block
from core.merkle import merkle_root
from core.block import Block

class Blockchain:
    def __init__(self):
        self.chain = [create_genesis_block()]
        self.difficulty = 1

    def mine_block(self, miner_address):
        prev = self.chain[-1]
        start = time.time()
        nonce = 0

        while True:
            root = merkle_root([])
            block = Block(
                index=len(self.chain),
                previous_hash=prev.hash,
                timestamp=int(time.time()),
                transactions=[],
                nonce=nonce,
                difficulty=self.difficulty,
                merkle_root=root
            )
            if valid_proof(block.hash, self.difficulty):
                break
            nonce += 1

        actual_time = int(time.time() - start)
        reward = calculate_reward(self.difficulty, actual_time)
        self.difficulty = adjust_difficulty(self.difficulty, actual_time)
        self.chain.append(block)
        return block, reward
