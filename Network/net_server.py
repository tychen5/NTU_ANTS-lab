import socket, sys
import time
from thread import *


def threadWork(client):
    while True:
        msg = client.recv(1024)
        if not msg:
            pass
        else:
                print time.asctime(time.localtime(time.time())), "Client send: " + msg
    client.close()

try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except socket.error, msg:
    sys.stderr.write("[ERROR] %s\n" % msg[1])
    sys.exit(1)

sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) #reuse tcp
sock.bind(('140.112.107.194', 11111))
sock.listen(5)
print("server on")

while True:
    (csock, adr) = sock.accept()
    print "Client Info: ", csock, adr
    start_new_thread(threadWork, (csock,))
