import hashlib

def merkle_root(transactions):
    if not transactions:
        return hashlib.sha256("empty".encode()).hexdigest()

    layer = [hashlib.sha256(str(tx).encode()).hexdigest() for tx in transactions]

    while len(layer) > 1:
        if len(layer) % 2 != 0:
            layer.append(layer[-1])

        new_layer = []
        for i in range(0, len(layer), 2):
            combined = layer[i] + layer[i+1]
            new_layer.append(hashlib.sha256(combined.encode()).hexdigest())

        layer = new_layer

    return layer[0]