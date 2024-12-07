import java.net.DatagramSocket;
import java.util.concurrent.ConcurrentLinkedQueue;

import NetLib.ClientTimeoutException;
import NetLib.PacketFlag;
import NetLib.S64Packet;
import NetLib.UDPHandler;

public class MasterConnectionThread extends Thread {

    private static final int TIME_HEARTBEAT = 1000*60*3;

    // Networking
    String address;
    int port;
    DatagramSocket socket;
    UDPHandler handler;
    
    // Thread communication
    ConcurrentLinkedQueue<byte[]> msgqueue = new ConcurrentLinkedQueue<byte[]>();
    
    /**
     * Thread for handling Master Server's UDP communication
     * @param socket   Socket to use for communication
     * @param address  Client address
     * @param port     Client port
     */
    MasterConnectionThread(DatagramSocket socket, String address, int port) {
        this.socket = socket;
        this.address = address;
        this.port = port;
        this.handler = null;
    }
    
    /**
     * Send a message to this thread
     * The message should be the raw bytes received from the client
     * @param data  The data received from the client
     * @param size  The size of the received data
     */
    public void SendMessage(byte data[], int size) {
        byte[] copy = new byte[size];
        System.arraycopy(data, 0, copy, 0, size);
        this.msgqueue.add(copy);
    }

    /**
     * Run this thread
     */
    public void run() {
        Thread.currentThread().setName("Master Server Connection");
        this.handler = new UDPHandler(this.socket, this.address, this.port);
        
        // Send the register packet to the master server
        try {
            this.handler.SendPacket(new S64Packet("REGISTER", TicTacToeServer.ToByteArray_Master(), PacketFlag.FLAG_EXPLICITACK.GetInt()));
            while (true) {
                S64Packet pkt;
                byte[] reply = null;
                
                // Keep sending until we got an ACK from the master server
                while (reply == null) {
                    Thread.sleep(UDPHandler.TIME_RESEND);
                    reply = this.msgqueue.poll();
                    if (reply == null)
                        this.handler.ResendMissingPackets();
                }
                pkt = this.handler.ReadS64Packet(reply);
                if (pkt == null)
                    continue;
                if (pkt.GetType().equals("ACK"))
                    break;
            }
            System.out.println("Register successful.");
        } catch (Exception e) {
            System.err.println("Unable to register to master server -> " + e);
            return;
        }
        
        // Send heartbeat packets every 5 minutes
        while (true) {
            try {
                Thread.sleep(TIME_HEARTBEAT);
                this.handler.SendPacket(new S64Packet("HEARTBEAT", TicTacToeServer.ToByteArray_Master(), PacketFlag.FLAG_EXPLICITACK.GetInt()));
                while (true) {
                    S64Packet pkt;
                    byte[] reply = null;
                    
                    // Keep sending until we got an ACK from the master server
                    while (reply == null) {
                        Thread.sleep(UDPHandler.TIME_RESEND);
                        reply = this.msgqueue.poll();
                        if (reply == null)
                            this.handler.ResendMissingPackets();
                    }
                    pkt = this.handler.ReadS64Packet(reply);
                    if (pkt == null)
                        continue;
                    if (pkt.GetType().equals("ACK"))
                        break;
                }
            } catch (ClientTimeoutException e) {
                System.err.println("Master server did not respond to heartbeat.");
                break;
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
}