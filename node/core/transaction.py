import uuid
import json

def create_transaction(sender, receiver, amount, nonce, signature):
    tx = {
        "id": str(uuid.uuid4()),
        "sender": sender,
        "receiver": receiver,
        "amount": amount,
        "nonce": nonce,
        "signature": signature
    }
    return tx

def tx_to_string(tx):
    return json.dumps({
        "sender": tx["sender"],
        "receiver": tx["receiver"],
        "amount": tx["amount"],
        "nonce": tx["nonce"]
    }, sort_keys=True)