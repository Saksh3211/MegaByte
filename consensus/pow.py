def valid_proof(block_hash, difficulty):
    return block_hash.startswith("0" * difficulty)
