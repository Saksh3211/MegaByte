class State:
    def __init__(self):
        self.balances = {}
        self.nonces = {}

    def get_balance(self, address):
        return self.balances.get(address, 0)

    def get_nonce(self, address):
        return self.nonces.get(address, 0)

    def apply_transaction(self, tx:dict):
        sender = tx["sender"]
        receiver = tx["receiver"]
        amount = tx.get("amount",0)
        nonce = tx.get("nonce",0)

        if sender != "NETWORK":
            if self.get_balance(sender) < amount:
                return False

            if self.get_nonce(sender) != nonce:
                return False

            self.balances[sender] -= amount
            self.nonces[sender] = nonce + 1

        self.balances[receiver] = self.get_balance(receiver) + amount
        return True