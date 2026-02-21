import hashlib
import json

def sha256(data):
    return hashlib.sha256(data.encode()).hexdigest()

def hash_block(block_data):
    block_string = json.dumps(block_data, sort_keys=True)
    return sha256(block_string)