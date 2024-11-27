import java.net.DatagramSocket;
import java.util.concurrent.ConcurrentLinkedQueue;

import NetLib.ClientTimeoutException;
import NetLib.PacketFlag;
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
            while (true) {
                byte[] reply;
                this.handler.SendPacket(new S64Packet("REGISTER", TicTacToeServer.ToByteArray_Master(), PacketFlag.FLAG_EXPLICITACK.GetInt()));
                reply = this.msgqueue.poll();
                while (reply == null) {
                    Thread.sleep(UDPHandler.TIME_RESEND);
                    this.handler.ResendMissingPackets();
                    reply = this.msgqueue.poll();
                }
                if (!this.handler.IsS64Packet(reply)) {
                    System.err.println("Got bad reply from master server");
                    continue;
                }
                if (this.handler.ReadS64Packet(reply).GetType().equals("ACK"))
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
                while (true) {
                    byte[] reply;
                    this.handler.SendPacket(new S64Packet("HEARTBEAT", TicTacToeServer.ToByteArray_Master(), PacketFlag.FLAG_EXPLICITACK.GetInt()));
                    reply = this.msgqueue.poll();
                    while (reply == null) {
                        Thread.sleep(UDPHandler.TIME_RESEND);
                        this.handler.ResendMissingPackets();
                        reply = this.msgqueue.poll();
                    }
                    if (!this.handler.IsS64Packet(reply)) {
                        System.err.println("Got bad reply from master server");
                        continue;
                    }
                    if (this.handler.ReadS64Packet(reply).GetType().equals("ACK"))
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