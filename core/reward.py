import math
from config.constants import BASE_REWARD, BLOCK_TIME_TARGET

def calculate_reward(difficulty, actual_time):
    diff_factor = math.log(difficulty + 1, 2)
    time_factor = min(1.5, max(0.5, actual_time / BLOCK_TIME_TARGET))
    return round(BASE_REWARD * diff_factor * time_factor, 8)
