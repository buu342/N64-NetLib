import java.net.Socket;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.DataOutputStream;
import java.io.DataInputStream;
import NetLib.N64Server;
import NetLib.N64ROM;
import java.io.IOException;
import java.io.File;
import java.io.FileInputStream;
import java.io.BufferedInputStream;
import java.nio.ByteBuffer;

public class ClientConnectionThread implements Runnable {

	Hashtable<String, N64ROM> roms;
    Hashtable<String, N64Server> servers;
    Socket clientsocket;
    
    ClientConnectionThread(Hashtable<String, N64Server> servers, Hashtable<String, N64ROM> roms, Socket socket) {
        this.servers = servers;
        this.roms = roms;
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
                if (this.roms.get(server.GetROMHashStr()) != null)
                	dos.write(new byte[]{1});
                else
                	dos.write(new byte[]{0});
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
    	String romhashstr;
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
    	
    	// Check if this ROM exists. If it doesn't, search the ROM folder to confirm if it's since been added
    	romhashstr = N64ROM.BytesToHash(romhash);
    	if (this.roms.get(romhashstr) == null)
    		MasterServer.ValidateROM(romname);

    	// Store this server in our list
    	// If the server already exists, this will update it instead
    	this.servers.put(serveraddress, new N64Server(servername, maxcount, serveraddress, romname, romhash));
    }
    
    private void DownloadROM(byte[] data) throws Exception, IOException {
    	int size, readcount;
    	byte[] hash;
    	N64ROM rom;
    	String romhashstr;
    	File romfile;
    	byte[] buffer = new byte[8192];
    	ByteBuffer bb = ByteBuffer.wrap(data);
        DataOutputStream dos = new DataOutputStream(this.clientsocket.getOutputStream());
        BufferedInputStream bis;
        
        // Get the hash size
    	bb.position(8);
    	size = bb.getInt();
    	
    	// Store the hash
    	hash = new byte[size];
    	for (int i=0; i<size; i++)
    		hash[i] = bb.get();
    	
    	// Find the hash in our ROM list
    	romhashstr = N64ROM.BytesToHash(hash);
    	rom = this.roms.get(romhashstr);
    	if (rom == null)
    	{
    		System.out.println("Client requested non-existent ROM");
    		dos.writeInt(0);
    		dos.flush();
            dos.close();
    		return;
    	}
    	
    	// If the file doesn't exist any longer, delete it from our ROM list
    	romfile = new File(rom.GetPath());
    	if (!romfile.exists())
    	{
    		System.out.println("Client requested since-removed ROM");
    		this.roms.remove(romhashstr);
    		dos.writeInt(0);
    		dos.flush();
            dos.close();
    		return;
    	}
    	
    	// If the hash changed, update the hash and stop
    	if (N64ROM.GetROMHash(romfile).equals(hash) == false)
    	{
    		System.out.println("Client requested since-changed ROM");
			rom = new N64ROM(romfile);
    		this.roms.remove(romhashstr);
			this.roms.put(rom.GetHashString(), rom);
    		dos.writeInt(0);
    		dos.flush();
            dos.close();
    		return;
    	}
    	
    	// Transfer the ROM
    	dos.writeInt(rom.GetSize());
    	bis = new BufferedInputStream(new FileInputStream(romfile));
    	while ((readcount = bis.read(buffer)) > 0)
    		dos.write(buffer, 0, readcount);
        dos.flush();
    	
    	// Cleanup
    	bis.close();
        dos.close();
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
	        	} else if (CheckCString(data, "DOWNLOAD")) {
	        		this.DownloadROM(data);
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