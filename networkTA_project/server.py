import socket, sys
import time
from thread import *


def threadWork(client):
    global client_socker
    while True:
        msg = client.recv(1024)
        if not msg:
            pass
        else:
            if client_socker:
                for csocket in client_socker:
                    try:
                        csocket.sendall(msg)
                    except socket.error as e:
                        client_socker.remove(csocket)
    client.close()

# ========================== Socket initial ==========================
try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    csock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except socket.error, msg:
    sys.stderr.write("[ERROR] %s\n" % msg[1])
    sys.exit(1)

sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # reuse tcp
csock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

# ========================== Temu Client ==========================
sock.bind(('140.112.107.194', 10001))

sock.listen(5)
print("server on")

client_socker = []

while True:
    (csock, adr) = sock.accept()
    print "Client Info: ", csock, adr
    if adr[0].split(".")[0] in ['10', '172', '192']:
        start_new_thread(threadWork, (csock,))
        continue
    client_socker.append(csock)
