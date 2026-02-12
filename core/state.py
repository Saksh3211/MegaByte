class State:
    def __init__(self):
        self.accounts = {}
        self.tokens = {}

    def get_balance(self, addr):
        return self.accounts.get(addr, {}).get("balance", 0)
