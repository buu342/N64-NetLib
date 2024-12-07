# Fake client for use with the NetLib TicTacToe server

import io
import sys
import time
import socket
import threading

# Server address
HOST = "127.0.0.1"
PORT = 6460

# Current version of the NetLib packets
NETLIB_VERSION = 1

# Packet types
PACKETID_ACKBEAT = 0
PACKETID_CLIENTCONNECT = 1
PACKETID_PLAYERINFO = 2
PACKETID_PLAYERDISCONNECT = 3
PACKETID_SERVERFULL = 4
PACKETID_PLAYERREADY = 5

# Global sequence numbers
g_sequencenum_local = 0
g_sequencenum_remote = 0

# Message queue for thread communication
thread_messagequeue = []

class Packet:
    """
    NetLib packet for network communication

    Parameters
    ----------
    version : int
        The version of the NetLib packet
    type : int
        The type of the packet
    recipients : int
        A bitmask with the list of players this packet is intended for
    data : int
        The payload data of the packet
    """
    def __init__(self, version, type, recipients, data):
        self.version = version
        self.type = type
        self.recipients = recipients
        self.data = data
        if (data == None):
            self.size = 0
        else:
            self.size = len(data)
        self.flags = 0
        self.sequencenum_local = 0
        self.sequencenum_remote = 0
        self.sequencenum_ackfield = 0xFFFF

    def __repr__(self):
        mystr = "NetLib Packet\n"
        mystr += "    Version: " + str(self.version) + "\n"
        mystr += "    Type: " + str(self.type) + "\n"
        mystr += "    Recipients: " + "{:032b}".format(self.recipients) + "\n"
        mystr += "    SeqNum: " + str(self.sequencenum_local) + "\n"
        mystr += "    RemoteNum: " + str(self.sequencenum_remote) + "\n"
        mystr += "    AckField: " + "{:016b}".format(self.sequencenum_ackfield) + "\n"
        if (self.size > 0):
            mystr += "    Data: \n"
            mystr += "    " + self.data.hex()
        return mystr

    def gettype(self):
        """
        Get the type of the packet

        Returns
        -------
        out : int
            The type of the packet
        """
        return self.type

    def receivepacket(self, sock):
        """
        Receive a packet from the internet

        Parameters
        ----------
        sock : socket
            The socket to read from

        Returns
        -------
        out : bool
            Whether or not we received a valid packet
        """
        global g_sequencenum_remote
        data, addr = sock.recvfrom(4096)

        datastream = io.BytesIO(data)
        if (datastream.read(3) == None):
            return False
        self.version = int.from_bytes(datastream.read(1), "big")

        self.type = int.from_bytes(datastream.read(1), "big")
        self.flags = int.from_bytes(datastream.read(1), "big")

        self.sequencenum_local = int.from_bytes(datastream.read(2), "big")
        self.sequencenum_remote = int.from_bytes(datastream.read(2), "big")
        self.sequencenum_ackfield = int.from_bytes(datastream.read(2), "big")
        if (g_sequencenum_remote < self.sequencenum_remote):
            g_sequencenum_remote = self.sequencenum_remote

        self.recipients = int.from_bytes(datastream.read(4), "big")

        self.size = int.from_bytes(datastream.read(2), "big")
        if (self.size > 0):
            self.data = datastream.read(self.size)

        return True

    def sendpacket(self, sock):
        """
        Sends a packet over the internet

        Parameters
        ----------
        sock : socket
            The socket to write to
        """
        global g_sequencenum_local
        global g_sequencenum_remote
        self.sequencenum_local = g_sequencenum_local
        self.sequencenum_remote = g_sequencenum_remote
        g_sequencenum_local += 1
        data = bytearray()
        data += bytearray(b'\x4E')
        data += bytearray(b'\x4C')
        data += bytearray(b'\x50')
        data += bytearray(self.version.to_bytes(length=1))
        data += bytearray(self.type.to_bytes(length=1))
        data += bytearray(self.flags.to_bytes(length=1))
        data += bytearray(self.sequencenum_local.to_bytes(length=2, byteorder='big'))
        data += bytearray(self.sequencenum_remote.to_bytes(length=2, byteorder='big'))
        data += bytearray(self.sequencenum_ackfield.to_bytes(length=2, byteorder='big'))
        data += bytearray(self.recipients.to_bytes(length=4, byteorder='big'))
        data += bytearray(self.size.to_bytes(length=2, byteorder='big'))
        if (self.size > 0):
            data += self.data
        sock.sendto(data, (HOST, PORT))

def inputthread():
    """
    A thread for handling user input for packets
    """
    while (True):
        try:
            x = input('Packet Input:')
            x = x.split()
            type = int(x.pop(0))
            recipients = 0 #int(x.pop(0))
            data = bytearray()
            for b in x:
                data += bytearray(int(b).to_bytes(length=1))
            p = Packet(NETLIB_VERSION, type, recipients, data)
            thread_messagequeue.append(p)
        except Exception as e:
            print(e)

def main():
    """
    Program entrypoint
    """
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.setblocking(True)
        s.bind(("", 0))

        ## Send the connection packet
        p = Packet(1, PACKETID_CLIENTCONNECT, 0, None)
        p.sendpacket(s)

        # Receive the player number packet
        p.receivepacket(s)

        # Create an input thread
        t = threading.Thread(target=inputthread, daemon=True)
        t.start()
        
        # Now loop forever
        s.setblocking(False)
        while (True):

            # If we have packets from the input thread, send them
            while (len(thread_messagequeue) > 0):
                p = thread_messagequeue.pop(0)
                p.sendpacket(s)

            # Check for packets from the server and print them (or respond to ackbeats)
            try:
                p = Packet(NETLIB_VERSION, 0, 0, None)
                if not (p.receivepacket(s)):
                    continue
                if (p.gettype() == PACKETID_ACKBEAT):
                    p = Packet(NETLIB_VERSION, PACKETID_ACKBEAT, 0, None)
                    p.sendpacket(s)
                else:
                    print(p)
            except Exception as e:
                #print(e)
                time.sleep(0.01)
        sys.exit()

# Start the program
main()