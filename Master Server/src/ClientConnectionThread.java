import java.net.Socket;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.DataOutputStream;
import java.io.DataInputStream;
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
            if (serverbytes != null) {
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
    
    private boolean CheckCString(byte[] data, String str)
    {
    	for (int i=0; i<data.length; i++) {
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
        	while (true)
        	{
        		int datasize = 0;
	        	byte[] data = dis.readNBytes(6);
	        	if (!CheckCString(data, "N64PKT")) {
	        		System.out.println("Received bad packet "+data.toString());
	        		attempts--;
	        		if (attempts == 0) {
	        			System.out.println("Too many bad packets from client "+this.clientsocket+". Disconnecting");
	        			break;
	        		}
	        		continue;
	        	}
	        	datasize = dis.readInt();
	        	System.out.println("Packet from client has size " + String.valueOf(datasize));
	        	data = dis.readNBytes(datasize);
	        	if (CheckCString(data, "LIST")) {
	        		this.ListServers();
	        		break;
	        	}
        	}
            System.out.println("Finished with "+this.clientsocket);
            this.clientsocket.close();
            dis.close();
        } catch (Exception e) {
            System.err.println(e);
        }
    }
}