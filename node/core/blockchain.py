import time
import threading
from core.block import Block
from consensus.pow import proof_of_work
from config import DIFFICULTY, BLOCK_REWARD, BLOCK_TIME
from consensus.difficulty import adjust_difficulty

class Blockchain:
    def __init__(self, state, mempool):
        self.chain = []
        self.state = state()
        self.mempool = mempool()
        self.last_block_time = time.time()
        self.create_genesis()
        self.mining = False
        self.difficulty = DIFFICULTY
        self.accounts = {}

    def create_genesis(self):
        genesis = Block(0, [], "0", DIFFICULTY)
        genesis.hash = proof_of_work(genesis)
        self.chain.append(genesis)

    def get_last_block(self):
        return self.chain[-1]

    def mine_loop(self, miner_address):
        self.mining = True

        while self.mining:
            if time.time() - self.last_block_time >= BLOCK_TIME:

                txs = self.mempool.get_transactions()

                reward_tx = {
                    "sender": "NETWORK",
                    "receiver": miner_address,
                    "amount": BLOCK_REWARD,
                    "nonce": 0
                }

                txs.append(reward_tx)

                block = Block(
                    len(self.chain),
                    txs,
                    self.get_last_block().hash,
                    DIFFICULTY
                )

                block.hash = proof_of_work(block)

                for tx in txs:
                    self.state.apply_transaction(tx)

                self.chain.append(block)
                self.mempool.clear()
                self.last_block_time = time.time()

                print(f"Block {block.index} mined!")
                self.difficulty = adjust_difficulty(self)
            time.sleep(1)

    def start_mining(self, miner_address):
        thread = threading.Thread(target=self.mine_loop, args=(miner_address,))
        thread.daemon = True
        thread.start()
    
    def update_balances(self, transactions):
        for tx in transactions:
            sender = tx["sender"]
            receiver = tx["receiver"]
            amount = int(tx["amount"])

            if sender != "NETWORK":
                self.accounts[sender] = self.accounts.get(sender, 0) - amount

            self.accounts[receiver] = self.accounts.get(receiver, 0) + amount