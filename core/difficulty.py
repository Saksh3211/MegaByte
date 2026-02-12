from config.constants import BLOCK_TIME_TARGET

def adjust_difficulty(prev_difficulty, actual_time):
    if actual_time <= 0:
        return prev_difficulty
    ratio = BLOCK_TIME_TARGET / actual_time
    return max(1, int(prev_difficulty * ratio))
