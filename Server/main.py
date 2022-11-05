import logging
from webbrowser import get
from xmlrpc.client import Server
import server


PORT_INFO_PATH = "port.info"
HOST = "127.0.0.1"  # Standard loopback interface address (localhost)


def get_port():
    try:
        with open(PORT_INFO_PATH, 'r') as file:
            port = file.readline().strip()
            return int(port)
    except FileNotFoundError:
        return None


def main():
    port = get_port()
    if not port:
        logging.error()
    s = server.Server("127.0.0.1", port)
    
    if not s.listen():
        logging.error("Exit")
        exit(1)



if __name__ == "__main__":
    main()
