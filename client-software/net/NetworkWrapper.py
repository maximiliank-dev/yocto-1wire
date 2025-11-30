from socket import *


class NetworkWrapper:
    """Wrapper for the TCP client
       Simplified usage"""

    client_setup = False
    buffer_length = 1024

    def __init__(self, host, port):
        try:
            self.client = socket(AF_INET, SOCK_STREAM)
            self.client.connect((host, int(port)))
            self.client_setup = True
        except Exception as e:
            print(e)
        
    def send(self, message : str):
        self.client.sendall(message.encode("utf-8"))

    def receive(self) -> bytes:
        data = self.client.recv(self.buffer_length)

        return data

    def close(self):
        self.client.close()
