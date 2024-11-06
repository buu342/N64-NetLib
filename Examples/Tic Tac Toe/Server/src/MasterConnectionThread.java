import java.net.Socket;

import NetLib.S64Packet;

import java.io.DataOutputStream;

public class MasterConnectionThread implements Runnable {
    
    private static final int HEARTBEAT = 5000;

    Socket mastersocket;
    
    MasterConnectionThread(Socket socket) {
        this.mastersocket = socket;
    }
    
    public void run() {
        DataOutputStream dos;
        
        // Open the data output stream
        try {
            dos = new DataOutputStream(this.mastersocket.getOutputStream());
        } catch (Exception e) {
            System.err.println("Failed to open data output stream to master server.");
            e.printStackTrace();
            return;
        }
        
        // Register to the master server
        try {
            byte[] serverbytes = TicTacToeServer.ToByteArray();
            S64Packet pkt = new S64Packet("REGISTER", serverbytes);
            pkt.WritePacket(dos);
            System.out.println("Success.");
        } catch (Exception e) {
            System.err.println("Unable to register to master server.");
            e.printStackTrace();
        }
        
        // Do a periodic heartbeat
        try {
            while (!this.mastersocket.isClosed())
            {
                S64Packet pkt = new S64Packet("PING", null);
                pkt.WritePacket(dos);
                Thread.sleep(HEARTBEAT);
            }
            dos.close();
        } catch (Exception e) {
            System.err.println("Failed to send master server a heartbeat.");
            e.printStackTrace();
        }
        
    }
}