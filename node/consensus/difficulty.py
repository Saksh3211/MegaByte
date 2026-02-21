from config import BLOCK_TIME,DIFFICULTY_Adjust

def adjust_difficulty(blockchain):
    if len(blockchain.chain) % DIFFICULTY_Adjust != 0:
        return blockchain.difficulty

    latest = blockchain.chain[-1]
    prev_adjustment_block = blockchain.chain[-DIFFICULTY_Adjust]

    time_taken = latest.timestamp - prev_adjustment_block.timestamp
    expected_time = BLOCK_TIME * DIFFICULTY_Adjust

    if time_taken < expected_time:
        return blockchain.difficulty + 1
    elif time_taken > expected_time:
        return max(1, blockchain.difficulty - 1)

    return blockchain.difficulty