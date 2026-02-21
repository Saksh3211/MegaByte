import json

def create_message(type_, data):
    return json.dumps({
        "type": type_,
        "data": data
    })