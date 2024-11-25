import java.net.DatagramSocket;
import java.util.concurrent.ConcurrentLinkedQueue;

import NetLib.ClientTimeoutException;
import NetLib.S64Packet;
import NetLib.UDPHandler;

public class MasterConnectionThread implements Runnable {

    private static final int TIME_HEARTBEAT = 1000*60*5;

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
        
        // Send heartbeat packets every 5 minutes
        while (true) {
            try {
                this.handler.SendPacketWaitAck(new S64Packet("HEARTBEAT", null), this.msgqueue);
                Thread.sleep(TIME_HEARTBEAT);
            } catch (ClientTimeoutException e) {
                System.err.println("Master server did not respond to heartbeat.");
                break;
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
}