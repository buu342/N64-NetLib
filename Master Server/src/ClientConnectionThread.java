import java.net.Socket;
import java.util.Hashtable;

import N64.N64ROM;
import N64.N64Server;
import NetLib.S64Packet;

import java.util.Enumeration;
import java.util.Arrays;
import java.io.DataOutputStream;
import java.io.DataInputStream;
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
        System.out.println("Client " + this.clientsocket + " requested list of servers");
        while (keys.hasMoreElements()) {
            String key = keys.nextElement();
            N64Server server = this.servers.get(key);
            byte[] serverbytes = server.toByteArray();
            if (serverbytes != null) {
                byte[] sendbytes = new byte[serverbytes.length + 1];
                System.arraycopy(serverbytes, 0, sendbytes, 0, serverbytes.length);
                if (this.roms.get(server.GetROMHashStr()) != null)
                    sendbytes[serverbytes.length] = 1;
                else
                    sendbytes[serverbytes.length] = 0;
                S64Packet pkt = new S64Packet("SERVER", sendbytes);
                pkt.WritePacket(dos);
            }
        }
        dos.close();
    }
    
    private void RegisterServer(byte[] data)
    {
        System.out.println("Client " + this.clientsocket + " requested server register");
        int size;
        int maxcount = 0;
        String serveraddress = this.clientsocket.getInetAddress().getHostAddress();
        int publicport = 0;
        String servername = "";
        String romname = "";
        String fullserveraddr;
        String romhashstr;
        byte[] romhash;
        N64Server server;
        ByteBuffer bb = ByteBuffer.wrap(data);
        bb.position(8);
        
        // Server name
        size = bb.getInt();
        for (int i=0; i<size; i++)
            servername += (char)bb.get();
        
        // Public port
        publicport = bb.getInt();
        
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
        
        fullserveraddr = serveraddress + ":" + publicport;

        // If a server already exists in this address, and for some reason the socket is still open, force close it
        server = this.servers.get(fullserveraddr);
        if (server != null) {
        	try {
        		server.GetSocket().close();
        	} catch (Exception e) {
                System.err.println("Unable to close already existing server socket.");
                e.printStackTrace();
        	}
        }
        
        // Store this server in our list
        // If the server already exists, this will update it instead
        server = new N64Server(servername, maxcount, serveraddress, publicport, romname, romhash, this.clientsocket);
        this.servers.put(fullserveraddr, server);
    
        // Keep the server connected and listen to heartbeat packets
        while (!this.clientsocket.isClosed()) {
        	try {
                Thread.sleep(10000);
            } catch (InterruptedException e) {
                e.printStackTrace();
                break;
            }
        }
        
        // Remove the server from the list, if it hasn't been overwritten (updated) by another server instance
        if (this.servers.get(fullserveraddr) == server)
        	this.servers.remove(fullserveraddr);
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
        
        System.out.println("Client " + this.clientsocket + " wants to download ROM");
        
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
            System.err.println("    Client requested non-existent ROM");
            dos.writeInt(0);
            dos.flush();
            dos.close();
            return;
        }
        
        // If the file doesn't exist any longer, delete it from our ROM list
        romfile = new File(rom.GetPath());
        if (!romfile.exists())
        {
            System.err.println("    Client requested since-removed ROM");
            this.roms.remove(romhashstr);
            dos.writeInt(0);
            dos.flush();
            dos.close();
            return;
        }
        
        // If the hash changed, update the hash and stop
        if (Arrays.equals(N64ROM.GetROMHash(romfile), hash) == false)
        {
            System.err.println("    Client requested since-changed ROM");
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
    
    public void run() {
        try {
            int attempts = 5;
            DataInputStream dis = new DataInputStream(this.clientsocket.getInputStream());
            while (true)
            {
                S64Packet pkt = S64Packet.ReadPacket(dis);
                if (pkt == null) {
                    System.err.println("    Received bad packet");
                    attempts--;
                    if (attempts == 0) {
                        System.err.println("Too many bad packets from client "+this.clientsocket+". Disconnecting");
                        break;
                    }
                    continue;
                }
                attempts = 5;
                if (pkt.GetType().equals("LIST")) {
                    this.ListServers();
                    break;
                } else if (pkt.GetType().equals("REGISTER")) {
                    this.RegisterServer(pkt.GetData());
                    break;
                } else if (pkt.GetType().equals("DOWNLOAD")) {
                    this.DownloadROM(pkt.GetData());
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