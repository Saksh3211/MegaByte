from crypto.hashing import sha256

def merkle_root(transactions):
    if not transactions:
        return sha256(b"")
    hashes = [sha256(str(tx).encode()) for tx in transactions]
    while len(hashes) > 1:
        hashes = [
            sha256((hashes[i] + hashes[i+1]).encode())
            for i in range(0, len(hashes), 2)
        ]
    return hashes[0]
