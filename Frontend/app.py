import customtkinter as ctk
import json
from api import send_transaction, mine_block, get_chain
from WalletManager import WalletManager

ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("blue")


class WalletApp(ctk.CTk):

    def __init__(self):
        super().__init__()
        self.title("MegaByte Wallet")
        self.geometry("900x600")

        self.wallet_manager = WalletManager()
        self.wallet = None

        self.show_login_screen()

    # =====================================================
    # LOGIN SCREEN
    # =====================================================

    def show_login_screen(self):
        self.clear_window()

        container = ctk.CTkFrame(self)
        container.pack(expand=True)

        ctk.CTkLabel(container, text="MegaByte Wallet",
                     font=("Arial", 26, "bold")).pack(pady=20)

        self.username_entry = ctk.CTkEntry(container, placeholder_text="Username", width=250)
        self.username_entry.pack(pady=10)

        self.password_entry = ctk.CTkEntry(container, placeholder_text="Password",
                                           show="*", width=250)
        self.password_entry.pack(pady=10)

        ctk.CTkButton(container, text="Login",
                      command=self.login_wallet, width=250).pack(pady=5)

        ctk.CTkButton(container, text="Create Account",
                      command=self.create_wallet, width=250).pack(pady=5)

        self.login_status = ctk.CTkLabel(container, text="")
        self.login_status.pack(pady=10)

    # =====================================================
    # MAIN APP UI
    # =====================================================

    def show_main_app(self):
        self.clear_window()

        # Top balance card
        self.balance_card = ctk.CTkFrame(self, height=140)
        self.balance_card.pack(fill="x", padx=20, pady=20)

        self.balance_label = ctk.CTkLabel(
            self.balance_card,
            text="Balance: 0 MBT",
            font=("Arial", 24, "bold")
        )
        self.balance_label.pack(pady=15)

        self.address_label = ctk.CTkLabel(
            self.balance_card,
            text=f"Address: {self.wallet['address']}",
            font=("Arial", 12)
        )
        self.address_label.pack()

        # Tabs
        self.tabs = ctk.CTkTabview(self)
        self.tabs.pack(expand=True, fill="both", padx=20, pady=10)

        self.home_tab = self.tabs.add("Home")
        self.send_tab = self.tabs.add("Send")
        self.mine_tab = self.tabs.add("Mine")
        self.settings_tab = self.tabs.add("Settings")

        self.build_home_tab()
        self.build_send_tab()
        self.build_mine_tab()
        self.build_settings_tab()

    # =====================================================
    # TAB BUILDERS
    # =====================================================

    def build_home_tab(self):
        ctk.CTkLabel(self.home_tab,
                     text="Welcome to MegaByte",
                     font=("Arial", 18)).pack(pady=20)

        self.refresh_btn = ctk.CTkButton(
            self.home_tab,
            text="Refresh Blockchain",
            command=self.download_chain
        )
        self.refresh_btn.pack(pady=10)

        self.output_box = ctk.CTkTextbox(self.home_tab, height=200)
        self.output_box.pack(pady=10, padx=20, fill="both", expand=True)

    def build_send_tab(self):
        ctk.CTkLabel(self.send_tab,
                     text="Send Transaction",
                     font=("Arial", 18)).pack(pady=20)

        self.receiver_entry = ctk.CTkEntry(
            self.send_tab, placeholder_text="Receiver Address", width=400)
        self.receiver_entry.pack(pady=10)

        self.amount_entry = ctk.CTkEntry(
            self.send_tab, placeholder_text="Amount", width=400)
        self.amount_entry.pack(pady=10)

        ctk.CTkButton(
            self.send_tab,
            text="Send",
            command=self.send_tx,
            width=200
        ).pack(pady=15)

    def build_mine_tab(self):
        ctk.CTkLabel(self.mine_tab,
                     text="Mining",
                     font=("Arial", 18)).pack(pady=20)

        ctk.CTkButton(
            self.mine_tab,
            text="Start Mining",
            command=self.mine,
            width=200
        ).pack(pady=15)

    def build_settings_tab(self):
        ctk.CTkLabel(self.settings_tab,
                     text="Settings",
                     font=("Arial", 18)).pack(pady=20)

        ctk.CTkButton(
            self.settings_tab,
            text="Logout",
            command=self.logout,
            width=200
        ).pack(pady=10)

    # =====================================================
    # AUTH METHODS
    # =====================================================

    def create_wallet(self):
        username = self.username_entry.get()
        password = self.password_entry.get()

        result = self.wallet_manager.create_wallet(username, password)

        if "error" in result:
            self.login_status.configure(text=result["error"])
        else:
            self.wallet = self.wallet_manager.current_wallet
            self.show_main_app()

    def login_wallet(self):
        username = self.username_entry.get()
        password = self.password_entry.get()

        result = self.wallet_manager.login(username, password)

        if "error" in result:
            self.login_status.configure(text=result["error"])
        else:
            self.wallet = self.wallet_manager.current_wallet
            self.show_main_app()

    def logout(self):
        self.wallet_manager.logout()
        self.wallet = None
        self.show_login_screen()

    # =====================================================
    # BLOCKCHAIN ACTIONS
    # =====================================================

    def mine(self):
        if not self.wallet:
            return

        result = mine_block(self.wallet["address"])
        self.output_box.insert("end", json.dumps(result, indent=2) + "\n")

    def send_tx(self):
        if not self.wallet:
            return

        tx = {
            "sender": self.wallet["address"],
            "receiver": self.receiver_entry.get(),
            "amount": self.amount_entry.get()
        }

        result = send_transaction(tx)
        self.output_box.insert("end", json.dumps(result, indent=2) + "\n")

    def download_chain(self):
        chain = get_chain()
        with open("blockchain_download.json", "w") as f:
            json.dump(chain, f, indent=2)

        self.output_box.insert("end", "Blockchain refreshed\n")

    # =====================================================
    # UTIL
    # =====================================================

    def clear_window(self):
        for widget in self.winfo_children():
            widget.destroy()


if __name__ == "__main__":
    app = WalletApp()
    app.mainloop()