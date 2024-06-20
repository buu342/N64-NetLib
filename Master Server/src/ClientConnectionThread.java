import java.net.Socket;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.DataOutputStream;
import java.io.ObjectOutputStream;
import NetLib.N64Server;

public class ClientConnectionThread implements Runnable {

    Hashtable<String, N64Server> servers;
    Socket clientsocket;
    
    ClientConnectionThread(Hashtable<String, N64Server> servers, Socket socket) {
        this.servers = servers;
        this.clientsocket = socket;
    }
    
    public void run() {
        try {
            DataOutputStream dos = new DataOutputStream(this.clientsocket.getOutputStream());
            Enumeration<String> keys = this.servers.keys();
            while (keys.hasMoreElements()) {
                String key = keys.nextElement();
                N64Server server = this.servers.get(key);
                ObjectOutputStream out = new ObjectOutputStream(dos);
                out.writeObject(server);
                out.close();
                dos.flush();
                Thread.sleep(1000);
            }
            dos.close();
            this.clientsocket.close();
        } catch (Exception e) {
            System.err.println("Error closing client socket.");
        }
    }
}