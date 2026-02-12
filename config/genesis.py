from core.block import Block
from core.merkle import merkle_root

def create_genesis_block():
    txs = []
    root = merkle_root(txs)
    return Block(
        index=0,
        previous_hash="0"*64,
        timestamp=0,
        transactions=txs,
        nonce=0,
        difficulty=1,
        merkle_root=root
    )
