package NetLib;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.util.LinkedList;
import java.util.concurrent.ConcurrentLinkedQueue;

public class UDPHandler {
    
    private final int TIME_TIMEOUT  = 30000;
    private final int TIME_ACKRETRY = 5000;
    
    String address;
    int port;
    DatagramSocket socket;
    int localseqnum; // Java doesn't support unsigned shorts, so we'll have to mimic them with ints
    int remoteseqnum;
    int ackbitfield;
    LinkedList<Integer> acksleft;
    
    public UDPHandler(DatagramSocket socket, String address, int port) {
        this.socket = socket;
        this.address = address;
        this.port = port;
        this.localseqnum = 0;
        this.remoteseqnum = 0;
        this.ackbitfield = 0;
        this.acksleft = new LinkedList<>();
    }

    boolean ShortGreaterThan(int s1, int s2) {
        return ((s1 > s2) && (s1 - s2 <= 32768)) || ((s1 < s2) && (s2 - s1 > 32768));
    }
    
    public String GetAddress() {
        return this.address;
    }

    public int GetPort() {
        return this.port;
    }
    
    public boolean IsS64Packet(byte[] data) {
        return S64Packet.IsS64PacketHeader(data);
    }
    
    public boolean IsNetLibPacket(byte[] data) {
        return NetLibPacket.IsNetLibPacketHeader(data);
    }
    
    public void SendPacket(S64Packet pkt) throws IOException {
        byte[] data;
        DatagramPacket out;
        short ackbitfield = 0;
        
        // Set the sequence data
        pkt.SetSequenceNumber((short)this.localseqnum);
        pkt.SetAck((short)this.remoteseqnum);
        for (Integer ack : this.acksleft)
            if (ShortGreaterThan(this.remoteseqnum, ack.intValue()))
                ackbitfield |= 1 << ((this.remoteseqnum - ack.intValue()) - 1);
        pkt.SetAckBitfield(ackbitfield);
        
        // Send the packet
        data = pkt.GetBytes();
        out= new DatagramPacket(data, data.length, InetAddress.getByName(this.address), this.port);
        this.socket.send(out);
        
        // Increase the local sequence number. We want a short, so we have to modulus it by the max 16 bit value
        this.localseqnum = (this.localseqnum + 1) % (0xFFFF + 1);
    }
    
    public void SendPacketWaitAck(S64Packet pkt, ConcurrentLinkedQueue<byte[]> msgqueue) throws IOException, InterruptedException, ClientTimeoutException {
        long packettime = 0;
        while (true) {
            S64Packet ack;
            byte[] response;
            this.SendPacket(pkt);
            packettime = System.currentTimeMillis();
            
            // Wait for a response
            response = msgqueue.poll();
            while (response == null) {
                Thread.sleep(10);
                if ((packettime - System.currentTimeMillis()) > TIME_ACKRETRY)
                    break;
                if ((packettime - System.currentTimeMillis()) > TIME_TIMEOUT)
                    throw new ClientTimeoutException(this.address + ":" + this.port);
                response = msgqueue.poll();
            }
            if (response == null || !this.IsS64Packet(response))
                continue;
            
            // If we got an ack, we're done
            ack = this.ReadS64Packet(response);
            if (ack != null && ack.GetType().equals("ACK"))
                return;
        }
    }
    
    // TODO:
    //public void SendPacket(NetLibPacket pkt) {
    //    
    //}
    
    public S64Packet ReadS64Packet(byte[] data) throws IOException {
        S64Packet pkt;
        if (!S64Packet.IsS64PacketHeader(data))
            return null;
        pkt = S64Packet.ReadPacket(data);
        if (pkt != null) {
            if (ShortGreaterThan(pkt.GetSequenceNumber(), this.remoteseqnum)) {
                this.remoteseqnum = pkt.GetSequenceNumber();
            }
            if (this.acksleft.size() > 17)
                this.acksleft.removeFirst();
            this.acksleft.addLast(Integer.valueOf(pkt.GetSequenceNumber()));
        }
        return pkt;
    }

    // TODO:
    //public NetLibPacket ReadNetLibPacket(byte[] data) throws IOException {
    //    if (!NetLibPacket.IsNetLibPacketHeader(data))
    //        return null;
    //    return NetLibPacket.ReadPacket(data);
    //}

}