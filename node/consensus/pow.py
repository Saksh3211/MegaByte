def proof_of_work(block):
    while not block.compute_hash().startswith("0" * block.difficulty):
        block.nonce += 1
    return block.compute_hash()