from core.blockchain import Blockchain
from core.mempool import Mempool
from core.state import State
from rpc.server import start_rpc

if __name__ == "__main__":
    start_rpc()

"""
def main(add):
    mempool = Mempool()
    state = State()
    blockchain = Blockchain(state, mempool)

    print("Enter miner address (public key):")
    miner_address = add or input()

    blockchain.start_mining(miner_address)

    app = create_server(blockchain, mempool, state)
    app.run(host=HOST, port=PORT)

if __name__ == "__main__":
    main(None)

"""