import java.net.Socket;

import NetLib.S64Packet;

import java.io.DataOutputStream;

public class MasterConnectionThread implements Runnable {

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
    }
}