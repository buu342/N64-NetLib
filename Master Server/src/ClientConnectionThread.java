import java.net.Socket;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.DataOutputStream;
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
                Thread.sleep(1000);
            }
            System.out.println("Finished with "+this.clientsocket);
            dos.close();
            this.clientsocket.close();
        } catch (Exception e) {
            System.err.println(e);
        }
    }
}