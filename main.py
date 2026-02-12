from core.blockchain import Blockchain

if __name__ == "__main__":
    bc = Blockchain()
    while True:
        block, reward = bc.mine_block("MINER_ADDRESS")
        print(f"Block {block.index} mined | reward {reward} MBC | diff {bc.difficulty}")
