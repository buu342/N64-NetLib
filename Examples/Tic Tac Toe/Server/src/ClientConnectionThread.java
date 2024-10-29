import java.net.Socket;
import java.io.DataInputStream;

public class ClientConnectionThread implements Runnable {

    Socket clientsocket;
    
    ClientConnectionThread(Socket socket) {
        this.clientsocket = socket;
    }
    
    private boolean CheckCString(byte[] data, String str)
    {
        int max = Math.min(str.length(), data.length);
        for (int i=0; i<max; i++) {
            char readbyte = (char) (((int) data[i]) & 0xFF);
            if (readbyte != str.charAt(i))
                return false;
        }
        return true;
    }
    
    public void run() {
        try {
            int attempts = 5;
            DataInputStream dis = new DataInputStream(this.clientsocket.getInputStream());
            while (true) {
                int datasize = 0;
                byte[] data = dis.readNBytes(6);
                if (!CheckCString(data, "N64PKT")) {
                    System.err.println("    Received bad packet "+data.toString());
                    attempts--;
                    if (attempts == 0) {
                        System.err.println("Too many bad packets from client "+this.clientsocket+". Disconnecting");
                        break;
                    }
                    continue;
                }
                datasize = dis.readInt();
                data = dis.readNBytes(datasize);
                
                // Check packets here
            }
            System.out.println("Finished with "+this.clientsocket);
            this.clientsocket.close();
            dis.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}