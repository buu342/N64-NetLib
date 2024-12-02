package NetLib;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.LinkedList;

public class UDPHandler {
    
    public static final int TIME_RESEND   = 1000;
    public static final int MAX_RESEND    = 5;
    
    String address;
    int port;
    DatagramSocket socket;
    int localseqnum_s64; // Java doesn't support unsigned shorts, so we'll have to mimic them with ints
    int remoteseqnum_s64;
    int ackbitfield_s64;
    LinkedList<S64Packet> acksleft_rx_s64;
    LinkedList<S64Packet> acksleft_tx_s64;
    int localseqnum_nlp;
    int remoteseqnum_nlp;
    int ackbitfield_nlp;
    LinkedList<NetLibPacket> acksleft_rx_nlp;
    LinkedList<NetLibPacket> acksleft_tx_nlp;
    
    public UDPHandler(DatagramSocket socket, String address, int port) {
        this.socket = socket;
        this.address = address;
        this.port = port;
        this.localseqnum_s64 = 0;
        this.remoteseqnum_s64 = 0;
        this.ackbitfield_s64 = 0;
        this.acksleft_rx_s64 = new LinkedList<>();
        this.acksleft_tx_s64 = new LinkedList<>();
        this.localseqnum_nlp = 0;
        this.remoteseqnum_nlp = 0;
        this.ackbitfield_nlp = 0;
        this.acksleft_rx_nlp = new LinkedList<>();
        this.acksleft_tx_nlp = new LinkedList<>();
    }
    
    public String GetAddress() {
        return this.address;
    }

    public int GetPort() {
        return this.port;
    }
    
    public void SendPacket(S64Packet pkt) throws IOException, ClientTimeoutException {
        byte[] data;
        DatagramPacket out;
        short ackbitfield = 0;
        
        // Check for timeouts
        pkt.UpdateSendAttempt();
        if (pkt.GetSendAttempts() > MAX_RESEND)
            throw new ClientTimeoutException(this.address);
        
        // Set the sequence data
        pkt.SetSequenceNumber((short)this.localseqnum_s64);
        pkt.SetAck((short)this.remoteseqnum_s64);
        for (S64Packet pkt2ack : this.acksleft_rx_s64)
            if (S64Packet.SequenceGreaterThan(this.remoteseqnum_s64, pkt2ack.GetSequenceNumber()))
                ackbitfield |= 1 << (NetLibPacket.SequenceDelta(this.remoteseqnum_s64, pkt2ack.GetSequenceNumber()) - 1);
        pkt.SetAckBitfield(ackbitfield);
        
        // Send the packet
        data = pkt.GetBytes();
        out = new DatagramPacket(data, data.length, InetAddress.getByName(this.address), this.port);
        this.socket.send(out);
        
        // Add it to our list of packets that need an ack
        if ((pkt.GetFlags() & PacketFlag.FLAG_UNRELIABLE.GetInt()) == 0 && pkt.GetSendAttempts() == 1) {
            this.acksleft_tx_s64.add(pkt);
        
            // Increase the local sequence number
            this.localseqnum_s64 = S64Packet.SequenceIncrement(this.localseqnum_s64);
        }
    }
    
    public void SendPacket(NetLibPacket pkt) throws IOException, ClientTimeoutException {
        byte[] data;
        DatagramPacket out;
        short ackbitfield = 0;
        
        // Check for timeouts
        pkt.UpdateSendAttempt();
        if (pkt.GetSendAttempts() > MAX_RESEND)
            throw new ClientTimeoutException(this.address);
        
        // Set the sequence data
        pkt.SetSequenceNumber((short)this.localseqnum_nlp);
        pkt.SetAck((short)this.remoteseqnum_nlp);
        for (NetLibPacket pkt2ack : this.acksleft_rx_nlp)
            if (NetLibPacket.SequenceGreaterThan(this.remoteseqnum_nlp, pkt2ack.GetSequenceNumber()))
                ackbitfield |= 1 << (NetLibPacket.SequenceDelta(this.remoteseqnum_nlp, pkt2ack.GetSequenceNumber()) - 1);
        pkt.SetAckBitfield(ackbitfield);
        
        // Send the packet
        data = pkt.GetBytes();
        out = new DatagramPacket(data, data.length, InetAddress.getByName(this.address), this.port);
        this.socket.send(out);
        
        // Add it to our list of packets that need an ack
        if ((pkt.GetFlags() & PacketFlag.FLAG_UNRELIABLE.GetInt()) == 0 && pkt.GetSendAttempts() == 1) {
            this.acksleft_tx_nlp.add(pkt);
        
            // Increase the local sequence number
            this.localseqnum_nlp = NetLibPacket.SequenceIncrement(this.localseqnum_nlp);
        }
    }
    
    public S64Packet ReadS64Packet(byte[] data) throws IOException, ClientTimeoutException {
        S64Packet pkt;
        if (!S64Packet.IsS64PacketHeader(data))
            return null;
        pkt = S64Packet.ReadPacket(data);
        if (pkt != null) {
            LinkedList<S64Packet> found_nlp = new LinkedList<S64Packet>();
            
            // If a packet with this sequence number already exists in our RX list, ignore this packet
            for (S64Packet rxpkt : this.acksleft_rx_s64)
                if (rxpkt.GetSequenceNumber() == pkt.GetSequenceNumber())
                    return null;
            
            // Go through transmitted packets and remove all which were acknowledged in the one we received
            for (S64Packet pkt2ack : this.acksleft_tx_s64)
                if (pkt.IsAcked(pkt2ack.GetSequenceNumber()))
                    found_nlp.add(pkt2ack);
            this.acksleft_tx_s64.removeAll(found_nlp);
            
            // Increment the sequence number to the packet's highest value
            if (S64Packet.SequenceGreaterThan(pkt.GetSequenceNumber(), this.remoteseqnum_s64))
                this.remoteseqnum_s64 = pkt.GetSequenceNumber();
            
            // Handle reliable packets
            if ((pkt.GetFlags() & PacketFlag.FLAG_UNRELIABLE.GetInt()) == 0) {
                
                // Update our received packet list
                if (this.acksleft_rx_s64.size() > 17)
                    this.acksleft_rx_s64.removeFirst();
                this.acksleft_rx_s64.addLast(pkt);
            }
            
            // If the packet wants an explicit ack, send it
            if ((pkt.GetFlags() & PacketFlag.FLAG_EXPLICITACK.GetInt()) != 0)
                this.SendPacket(new S64Packet("ACK", null, PacketFlag.FLAG_UNRELIABLE.GetInt()));
        }
        return pkt;
    }

    public NetLibPacket ReadNetLibPacket(byte[] data) throws IOException, ClientTimeoutException {
        NetLibPacket pkt;
        if (!NetLibPacket.IsNetLibPacketHeader(data))
            return null;
        pkt = NetLibPacket.ReadPacket(data);
        if (pkt != null) {
            LinkedList<NetLibPacket> found_nlp = new LinkedList<NetLibPacket>();
            
            // If a packet with this sequence number already exists in our RX list, ignore this packet
            for (NetLibPacket rxpkt : this.acksleft_rx_nlp)
                if (rxpkt.GetSequenceNumber() == pkt.GetSequenceNumber())
                    return null;
            
            // Go through transmitted packets and remove all which were acknowledged in the one we received
            for (NetLibPacket pkt2ack : this.acksleft_tx_nlp)
                if (pkt.IsAcked(pkt2ack.GetSequenceNumber()))
                    found_nlp.add(pkt2ack);
            this.acksleft_tx_nlp.removeAll(found_nlp);
            
            // Increment the sequence number to the packet's highest value
            if (NetLibPacket.SequenceGreaterThan(pkt.GetSequenceNumber(), this.remoteseqnum_nlp))
                this.remoteseqnum_nlp = pkt.GetSequenceNumber();
            
            // Handle reliable packets
            if ((pkt.GetFlags() & PacketFlag.FLAG_UNRELIABLE.GetInt()) == 0) {
                
                // Update our received packet list
                if (this.acksleft_rx_nlp.size() > 17)
                    this.acksleft_rx_nlp.removeFirst();
                this.acksleft_rx_nlp.addLast(pkt);
            }
            
            // If the packet wants an explicit ack, send it
            if ((pkt.GetFlags() & PacketFlag.FLAG_EXPLICITACK.GetInt()) != 0)
                this.SendPacket(new NetLibPacket(0, null, PacketFlag.FLAG_UNRELIABLE.GetInt()));
        }
        return pkt;
    }
    
    public void ResendMissingPackets() throws IOException, ClientTimeoutException {
        for (S64Packet pkt2ack : this.acksleft_tx_s64)
            if (pkt2ack.GetSendTime() > TIME_RESEND)
                this.SendPacket(pkt2ack);
        for (NetLibPacket pkt2ack : this.acksleft_tx_nlp)
            if (pkt2ack.GetSendTime() > TIME_RESEND)
                this.SendPacket(pkt2ack);
    }

}