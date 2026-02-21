import requests

BASE_URL = "http://127.0.0.1:1010"

def get_chain():
    return requests.get(f"{BASE_URL}/get_chain").json()

def send_transaction(data):
    return requests.post(f"{BASE_URL}/new_transaction", json=data).json()

def mine_block(address):
    return requests.post(
        f"{BASE_URL}/mine",
        json={"address": address}
    ).json()