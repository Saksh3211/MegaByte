import sqlite3

class Database:
    def __init__(self):
        self.conn = sqlite3.connect("blockchain.db", check_same_thread=False)
        self.cursor = self.conn.cursor()
        self.cursor.execute("""
            CREATE TABLE IF NOT EXISTS blocks (
                id INTEGER PRIMARY KEY,
                hash TEXT
            )
        """)
        self.conn.commit()

    def save_block(self, block):
        self.cursor.execute("INSERT INTO blocks (hash) VALUES (?)", (block.hash,))
        self.conn.commit()