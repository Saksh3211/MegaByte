class Mempool:
    def __init__(self):
        self.transactions = []

    def add_tx(self, tx):
        self.transactions.append(tx)

    def get_transactions(self):
        return self.transactions

    def clear(self):
        self.transactions = []