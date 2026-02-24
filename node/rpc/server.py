from flask import Flask, request, jsonify
from core.blockchain import Blockchain
from config import HOST, PORT
from core.state import State
from core.mempool import Mempool

app = Flask(__name__)
blockchain = Blockchain(State(), Mempool())

def serialize_block(block):
    return {
        "index": block.index,
        "timestamp": block.timestamp,
        "transactions": block.transactions,
        "previous_hash": block.previous_hash,
        "nonce": block.nonce,
        "difficulty": block.difficulty,
        "hash": block.hash,
        "merkle_root": block.merkle_root
    }


def serialize_chain():
    return {
        "length": len(blockchain.chain),
        "chain": [serialize_block(block) for block in blockchain.chain]
    }

@app.route("/get_chain", methods=["GET"])
def get_chain():
    return jsonify(serialize_chain())


@app.route("/new_transaction", methods=["POST"])
def new_transaction():
    data = request.json

    if not data:
        return jsonify({"error": "Invalid transaction data"}), 400

    blockchain.mempool.add_transaction(data)
    return jsonify({"status": "Transaction added"}), 200


@app.route("/mine", methods=["POST"])
def mine():
    data = request.json
    miner_address = data.get("address") if data else None

    if not miner_address:
        return jsonify({"error": "Miner address required"}), 400

    blockchain.start_mining(miner_address)

    return jsonify({
        "status": "Mining started",
        "miner": miner_address
    }), 200

def start_rpc():
    app.run(host=HOST, port=PORT)