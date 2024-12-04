package NetLib;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.LinkedList;

public class UDPHandler {
    
    public static final int TIME_RESEND   = 1000;
    public static final int MAX_RESEND    = 5;
    
    private static final boolean DEBUGPRINTS = false;
    
    String address;
    int port;
    DatagramSocket socket;
    int localseqnum; // Java doesn't support unsigned shorts, so we'll have to mimic them with ints
    int remoteseqnum;
    int ackbitfield;
    LinkedList<AbstractPacket> acksleft_rx;
    LinkedList<AbstractPacket> acksleft_tx;
    
    public UDPHandler(DatagramSocket socket, String address, int port) {
        this.socket = socket;
        this.address = address;
        this.port = port;
        this.localseqnum = 0;
        this.remoteseqnum = 0;
        this.ackbitfield = 0;
        this.acksleft_rx = new LinkedList<>();
        this.acksleft_tx = new LinkedList<>();
    }
    
    public String GetAddress() {
        return this.address;
    }

    public int GetPort() {
        return this.port;
    }
    
    public void SendPacket(AbstractPacket pkt) throws IOException, ClientTimeoutException {
        byte[] data;
        DatagramPacket out;
        short ackbitfield = 0;
        
        // Check for timeouts
        pkt.UpdateSendAttempt();
        if (pkt.GetSendAttempts() > MAX_RESEND)
            throw new ClientTimeoutException(this.address);
        
        // Set the sequence data
        if (pkt.GetSendAttempts() == 1) {
            pkt.SetSequenceNumber((short)this.localseqnum);
            pkt.SetAck((short)this.remoteseqnum);
            for (AbstractPacket pkt2ack : this.acksleft_rx)
                if (S64Packet.SequenceGreaterThan(this.remoteseqnum, pkt2ack.GetSequenceNumber()))
                    ackbitfield |= 1 << (NetLibPacket.SequenceDelta(this.remoteseqnum, pkt2ack.GetSequenceNumber()) - 1);
            pkt.SetAckBitfield(ackbitfield);
        }
        
        // Send the packet
        data = pkt.GetBytes();
        out = new DatagramPacket(data, data.length, InetAddress.getByName(this.address), this.port);
        this.socket.send(out);
        
        // Add it to our list of packets that need an ack
        if ((pkt.GetFlags() & PacketFlag.FLAG_UNRELIABLE.GetInt()) == 0 && pkt.GetSendAttempts() == 1) {
            this.acksleft_tx.add(pkt);
        
            // Increase the local sequence number
            this.localseqnum = S64Packet.SequenceIncrement(this.localseqnum);
        }
        
        // Debug print for developers
        if (DEBUGPRINTS) {
            if (!pkt.IsAckBeat()) {
                if (pkt.GetSendAttempts() > 1)
                    System.out.print("Re");
                System.out.print("Sent ");
                System.out.println(pkt);
            }
        }
    }
    
    private boolean HandlePacketSequence(AbstractPacket pkt, AbstractPacket ackbeat) throws IOException, ClientTimeoutException
    {
        LinkedList<AbstractPacket> found = new LinkedList<AbstractPacket>();
        
        // If a packet with this sequence number already exists in our RX list, ignore this packet
        for (AbstractPacket rxpkt : this.acksleft_rx) {
            if (rxpkt.GetSequenceNumber() == pkt.GetSequenceNumber()) {
                this.SendPacket(ackbeat);
                return false;
            }
        }
        
        // Go through transmitted packets and remove all which were acknowledged in the one we received
        for (AbstractPacket pkt2ack : this.acksleft_tx)
            if (pkt.IsAcked(pkt2ack.GetSequenceNumber()))
                found.add(pkt2ack);
        this.acksleft_tx.removeAll(found);
        
        // Increment the sequence number to the packet's highest value
        if (AbstractPacket.SequenceGreaterThan(pkt.GetSequenceNumber(), this.remoteseqnum))
            this.remoteseqnum = pkt.GetSequenceNumber();
        
        // Handle reliable packets
        if ((pkt.GetFlags() & PacketFlag.FLAG_UNRELIABLE.GetInt()) == 0) {
            
            // Update our received packet list
            if (this.acksleft_rx.size() > 17)
                this.acksleft_rx.removeFirst();
            this.acksleft_rx.addLast(pkt);
        }
        
        // If the packet wants an explicit ack, send it
        if ((pkt.GetFlags() & PacketFlag.FLAG_EXPLICITACK.GetInt()) != 0)
            this.SendPacket(ackbeat);
        return true;
    }
    
    public S64Packet ReadS64Packet(byte[] data) throws IOException, ClientTimeoutException {
        S64Packet pkt;
        if (!S64Packet.IsS64PacketHeader(data))
            return null;
        pkt = S64Packet.ReadPacket(data);

        // Handle sequence numbers
        if (pkt == null || !HandlePacketSequence(pkt, new S64Packet("ACK",  null, PacketFlag.FLAG_UNRELIABLE.GetInt())))
            return null;

        // Debug print for developers
        if (DEBUGPRINTS) {
            if (!pkt.IsAckBeat())
                System.out.println("Received " + pkt);
        }
        
        // Done
        return pkt;
    }

    public NetLibPacket ReadNetLibPacket(byte[] data) throws IOException, ClientTimeoutException {
        NetLibPacket pkt;
        if (!NetLibPacket.IsNetLibPacketHeader(data))
            return null;
        pkt = NetLibPacket.ReadPacket(data);

        // Handle sequence numbers
        if (pkt == null || !HandlePacketSequence(pkt, new NetLibPacket(0,  null, PacketFlag.FLAG_UNRELIABLE.GetInt())))
            return null;

        // Debug print for developers
        if (DEBUGPRINTS) {
            if (!pkt.IsAckBeat())
                System.out.println("Received " + pkt);
        }
        
        // Done
        return pkt;
    }
    
    public void ResendMissingPackets() throws IOException, ClientTimeoutException {
        for (AbstractPacket pkt2ack : this.acksleft_tx)
            if (pkt2ack.GetSendTime() > TIME_RESEND)
                this.SendPacket(pkt2ack);
    }

}