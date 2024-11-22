import java.net.DatagramSocket;
import java.util.concurrent.ConcurrentLinkedQueue;

import NetLib.S64Packet;
import NetLib.UDPHandler;

public class MasterConnectionThread implements Runnable {

    String address;
    int port;
    DatagramSocket socket;
    UDPHandler handler;
    ConcurrentLinkedQueue<byte[]> msgqueue = new ConcurrentLinkedQueue<byte[]>();
    
    MasterConnectionThread(DatagramSocket socket, String address, int port) {
        this.socket = socket;
        this.address = address;
        this.port = port;
        this.handler = null;
    }
    
    public void SendMessage(byte data[], int size) {
        byte[] copy = new byte[size];
        System.arraycopy(data, 0, copy, 0, size);
        this.msgqueue.add(copy);
    }
    
    public void run() {
        this.handler = new UDPHandler(this.socket, this.address, this.port);
        
        // Send the register packet to the master server
        try {
            S64Packet pkt = new S64Packet("REGISTER", TicTacToeServer.ToByteArray_Master());
            this.handler.SendPacketWaitAck(pkt, this.msgqueue);
            System.out.println("Register successful.");
        } catch (Exception e) {
            System.err.println("Unable to register to master server.");
            e.printStackTrace();
        }
        
        // Read packets from the master server
        while (true) {
            try {
                byte[] data = this.msgqueue.poll();
                if (data != null) {
                    if (!this.handler.IsS64Packet(data)) {
                        System.err.println("Received data which isn't an S64Packet from master server");
                        continue;
                    }
                    S64Packet pkt = this.handler.ReadS64Packet(data);
                    
                    if (pkt.GetType().equals("HEARTBEAT"))
                        this.handler.SendPacket(new S64Packet("ACK", null));
                } else {
                    Thread.sleep(500);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
}