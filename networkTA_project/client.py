import socket, sys


try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except socket.error, msg:
    sys.stderr.write("[ERROR] %s\n" % msg[1])
    sys.exit(1)

try:
    sock.connect(('140.112.107.194', 10001))
except socket.error, msg:
    sys.stderr.write("[ERROR] %s\n" % msg[1])
    exit(1)

while True:
    msg = sock.recv(1024)
    if not msg:
        pass
    else:
        print "Client send: " + msg
sock.close()
