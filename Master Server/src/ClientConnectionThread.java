import java.net.Socket;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.DataOutputStream;
import NetLib.N64Server;
import java.io.IOException;

public class ClientConnectionThread implements Runnable {

    Hashtable<String, N64Server> servers;
    Socket clientsocket;
    
    ClientConnectionThread(Hashtable<String, N64Server> servers, Socket socket) {
        this.servers = servers;
        this.clientsocket = socket;
    }
    
    private void ListServers() throws IOException
    {
        DataOutputStream dos = new DataOutputStream(this.clientsocket.getOutputStream());
        Enumeration<String> keys = this.servers.keys();
        System.out.println("Sending client "+this.clientsocket+" the list of servers");
        while (keys.hasMoreElements()) {
            String key = keys.nextElement();
            N64Server server = this.servers.get(key);
            byte[] serverbytes = server.toByteArray();
            if (serverbytes != null)
            {
            	final String packetype = "SERVER";
                dos.write("N64PKT".getBytes());
                dos.writeInt(serverbytes.length + packetype.length());
                dos.write(packetype.getBytes());
                dos.write(serverbytes);
                dos.flush();
            }
        }
        dos.close();
    }
    
    public void run() {
        try {
        	this.ListServers();
            System.out.println("Finished with "+this.clientsocket);
            this.clientsocket.close();
        } catch (Exception e) {
            System.err.println(e);
        }
    }
}