def create_token(state, creator, name, symbol, supply):
    token_id = f"{creator}:{symbol}"
    state.tokens[token_id] = {
        "name": name,
        "symbol": symbol,
        "supply": supply
    }
    state.accounts.setdefault(creator, {}).setdefault("tokens", {})[token_id] = supply
