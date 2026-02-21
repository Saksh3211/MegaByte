import socket
import threading
import json
from config import HOST, PORT

class P2PNode:
    def __init__(self, blockchain, host=HOST,port=PORT):
        self.blockchain = blockchain
        self.host = host
        self.port = port
        self.peers = []

    def start(self):
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.bind((self.host, self.port))
        server.listen()

        threading.Thread(target=self.accept_peers, args=(server,), daemon=True).start()

    def accept_peers(self, server):
        while True:
            conn, addr = server.accept()
            threading.Thread(target=self.handle_peer, args=(conn,), daemon=True).start()

    def handle_peer(self, conn):
        data = conn.recv(4096).decode()
        message = json.loads(data)

        if message["type"] == "REQUEST_CHAIN":
            response = json.dumps([block.__dict__ for block in self.blockchain.chain])
            conn.send(response.encode())

        conn.close()