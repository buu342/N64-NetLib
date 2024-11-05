import java.net.Socket;
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
            byte[] header = {'S', '6', '4', 'P', 'K', 'T'};
            byte[] packetype = {'R', 'E', 'G', 'I', 'S', 'T', 'E', 'R'};
            dos.write(header);
            // TODO: Send the packet version
            dos.writeInt(serverbytes.length + packetype.length);
            dos.write(packetype);
            dos.write(serverbytes);
            dos.flush();
        } catch (Exception e) {
            System.err.println("Unable to register to master server.");
            e.printStackTrace();
        }
        
        // Do a periodic heartbeat
        try {
            while (!this.mastersocket.isClosed())
            {
                byte[] header = {'S', '6', '4', 'P', 'K', 'T'};
                byte[] packetype = {'P', 'I', 'N', 'G'};
                dos.write(header);
                // TODO: Send the packet version
                dos.writeInt(packetype.length);
                dos.write(packetype);
                dos.flush();
                Thread.sleep(HEARTBEAT);
            }
            dos.close();
        } catch (Exception e) {
            System.err.println("Failed to send master server a heartbeat.");
            e.printStackTrace();
        }
        
    }
}