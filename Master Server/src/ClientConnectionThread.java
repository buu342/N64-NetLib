import java.net.Socket;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.DataOutputStream;
import java.io.DataInputStream;
import NetLib.N64Server;
import java.io.IOException;
import java.nio.ByteBuffer;

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
        System.out.println("Client "+this.clientsocket+" requested list of servers");
        while (keys.hasMoreElements()) {
            String key = keys.nextElement();
            N64Server server = this.servers.get(key);
            byte[] serverbytes = server.toByteArray();
            if (serverbytes != null) {
                byte[] header = {'N', '6', '4', 'P', 'K', 'T'};
            	byte[] packetype = {'S', 'E', 'R', 'V', 'E', 'R'};
                dos.write(header);
                dos.writeInt(serverbytes.length + packetype.length);
                dos.write(packetype);
                dos.write(serverbytes);
                dos.flush();
            }
        }
        dos.close();
    }
    
    private void RegisterServer(byte[] data)
    {
    	System.out.println("Client " + this.clientsocket + " requested server register");
    	int size;
    	int maxcount = 0;
    	String serveraddress = this.clientsocket.getRemoteSocketAddress().toString().replace("/","");
    	String servername = "";
    	String romname = "";
    	byte[] romhash;
    	ByteBuffer bb = ByteBuffer.wrap(data);
    	bb.position(8);
    	
    	// Server name
    	size = bb.getInt();
    	for (int i=0; i<size; i++)
    		servername += (char)bb.get();
    	
    	// Player max count
    	maxcount = bb.getInt();
    	
    	// ROM name
    	size = bb.getInt();
    	for (int i=0; i<size; i++)
    		romname += (char)bb.get();
    	
    	// ROM hash
    	size = bb.getInt();
    	romhash = new byte[size];
    	for (int i=0; i<size; i++)
    		romhash[i] += (char)bb.get();
    	
    	this.servers.put(serveraddress, new N64Server(servername, maxcount, serveraddress, romname, romhash));
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
	        	} else if (CheckCString(data, "REGISTER")) {
	        		this.RegisterServer(data);
	        		break;
	        	}
        	}
            System.out.println("Finished with "+this.clientsocket);
            this.clientsocket.close();
            dis.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}