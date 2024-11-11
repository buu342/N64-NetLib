import sys
import time
import socket
import threading

HOST = "127.0.0.1"
PORT = 6460

NETLIB_VERSION = 1

PACKETID_CLIENTCONNECT = 0
PACKETID_PLAYERINFO = 1
PACKETID_PLAYERDISCONNECT = 2
PACKETID_SERVERFULL = 3
PACKETID_HEARTBEAT = 4
PACKETID_PLAYERREADY = 5

thread_messagequeue = []

class Packet:
    def __init__(self, version, pid, recipients, data):
        self.version = version
        self.pid = pid
        self.recipients = recipients
        self.data = data
        if (data == None):
            self.size = 0
        else:
            self.size = len(data)

    def __repr__(self):
        mystr = "NetLib Packet\n"
        mystr += "    Version: " + str(self.version) + "\n"
        mystr += "    PID: " + str(self.pid) + "\n"
        mystr += "    Recipients: " + str(self.recipients) + "\n"
        mystr += "    Data: \n"
        mystr += "    " + self.data.hex()
        return mystr

    def getid(self):
        return self.pid

    def receive(self, sock):
        data = bytearray(3)
        if (sock.recv_into(data, 3) == 0):
            return False
        data = bytearray(1)
        sock.recv_into(data, 1)
        self.version = int.from_bytes(data, "big")
        data = bytearray(4)
        sock.recv_into(data, 4)
        self.size = int.from_bytes(data, "big")
        self.pid = (self.size & 0xFF000000) >> 24
        self.size &= 0x00FFFFFF
        data = bytearray(4)
        sock.recv_into(data, 4)
        self.recipients = int.from_bytes(data, "big")
        data = bytearray(self.size)
        sock.recv_into(data, self.size)
        self.data = data
        return True

    def send(self, sock):
        data = bytearray()
        data += bytearray(b'\x50')
        data += bytearray(b'\x4B')
        data += bytearray(b'\x54')
        data += bytearray(self.version.to_bytes(length=1))
        pack = ((self.pid << 24) & 0xFF000000) | (self.size & 0x00FFFFFF)
        data += bytearray(pack.to_bytes(length=4, byteorder='big'))
        data += bytearray(self.recipients.to_bytes(length=4, byteorder='big'))
        if (self.size > 0):
            data += self.data
        sock.sendall(data)

def inputthread():
    while (True):
        try:
            x = input('Packet Input:')
            x = x.split()
            pid = int(x.pop(0))
            recipients = 0 #int(x.pop(0))
            data = bytearray()
            for b in x:
                data += bytearray(int(b).to_bytes(length=1))
            p = Packet(NETLIB_VERSION, pid, recipients, data)
            thread_messagequeue.append(p)
        except Exception as e:
            print(e)

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setblocking(True)
        s.connect((HOST, PORT))

        ## Send the connection packet
        p = Packet(1, PACKETID_CLIENTCONNECT, 0, None)
        p.send(s)

        # Receive the player number packet
        p.receive(s)

        # Create an input thread
        t = threading.Thread(target=inputthread, daemon=True)
        t.start()
        
        # Now loop forever
        s.setblocking(False)
        while (True):
            while (len(thread_messagequeue) > 0):
                p = thread_messagequeue.pop(0)
                p.send(s)

            try:
                if not (p.receive(s)):
                    break;
                print(p)
                if (p.getid() == PACKETID_HEARTBEAT):
                    p = Packet(NETLIB_VERSION, PACKETID_HEARTBEAT, 0, None)
                    p.send(s)
            except:
                time.sleep(1)
        sys.exit()

main()