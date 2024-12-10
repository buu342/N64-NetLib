import N64.InvalidROMException;
import N64.N64ROM;
import N64.N64Server;
import NetLib.BadPacketVersionException;
import NetLib.ClientTimeoutException;
import NetLib.PacketFlag;
import NetLib.S64Packet;
import NetLib.UDPHandler;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.DatagramSocket;
import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

public class ClientConnectionThread extends Thread {
    
	// Constants
    private static final int FILECHUNKSIZE = 2048;

    // Connection handler
    private UDPHandler handler;
    
    // Database structs
    private ConcurrentHashMap<String, N64ROM> roms;
    private ConcurrentHashMap<String, N64Server> servers;

    // ROM download trackers
    private File romtodownload;
    private int romchunkcount;
    
    // Thread communication
    private ConcurrentLinkedQueue<byte[]> msgqueue = new ConcurrentLinkedQueue<byte[]>();
    
    /**
     * Thread for handling a client's UDP communication
     * @param servers  Server list database
     * @param roms     ROM list database
     * @param socket   Socket to use for communication
     * @param addr     Client address
     * @param port     Client port
     */
    ClientConnectionThread(ConcurrentHashMap<String, N64Server> servers, ConcurrentHashMap<String, N64ROM> roms, DatagramSocket socket, String addr, int port) {
        this.servers = servers;
        this.roms = roms;
        this.handler = new UDPHandler(socket, addr, port);
        this.romtodownload = null;
        this.romchunkcount = 0;
    }
    
    /**
     * Send a message to this thread
     * The message should be the raw bytes received from the client
     * @param data  The data received from the client
     * @param size  The size of the received data
     */
    public void SendMessage(byte data[], int size) {
        byte[] copy = new byte[size];
        System.arraycopy(data, 0, copy, 0, size);
        this.msgqueue.add(copy);
    }

    /**
     * Run this thread
     */
    public void run() {
        while (!Thread.currentThread().isInterrupted()) {
            try {
                // Check packets that came from a user (by reading the message queue)
                byte[] data = this.msgqueue.poll();
                if (data != null) {
                    S64Packet pkt = this.handler.ReadS64Packet(data);
                    if (pkt == null)
                        continue;
                    
                    // Handle each packet type
                    if (pkt.GetType().equals("REGISTER")) {
                        this.RegisterServer(pkt.GetData());
                        break;
                    } else if (pkt.GetType().equals("LIST")) {
                        this.ListServers();
                        break;
                    } else if (pkt.GetType().equals("DOWNLOAD")) {
                        this.DownloadROM(pkt.GetData());
                        continue;
                    } else if (pkt.GetType().equals("FILEDATA")) {
                        this.SendROMChunk(pkt.GetData());
                        continue;
                    } else if (pkt.GetType().equals("FILEDONE")) {
                        break;
                    } else if (pkt.GetType().equals("HEARTBEAT")) {
                        this.HandleServerHeartbeat();
                        break;
                    } else {
                        System.out.println("Received packet with unknown type '" + pkt.GetType() + "'");
                        break;
                    }
                } else {
                    Thread.sleep(10);
                }
            } catch (BadPacketVersionException e) {
                System.err.println(e);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
    
    /**
     * Handle a S64Packet with the type "REGISTER"
     * This packet is sent by servers in order to register with the master server
     * @param data  The packet data
     */
    private void RegisterServer(byte[] data)  {
        int size;
        int publicport = 0;
        String romname = "";
        String romhashstr;
        byte[] romhash;
        N64Server server;
        ByteBuffer bb = ByteBuffer.wrap(data);
        String addrport = this.handler.GetAddress() + ":" + this.handler.GetPort();
        System.out.println("Client " + addrport + " requested server register");
        
        // Public port
        publicport = bb.getInt();

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
            MasterServer.FindROMWithName(romname);
        
        // Store this server in our list
        // If the server already exists, this will update it instead
        server = new N64Server(this.handler.GetAddress(), publicport, romname, romhash);
        this.servers.put(addrport, server);
        System.out.println("Client " + addrport + " registered successfully");
    }

    /**
     * Handle a S64Packet with the type "LIST"
     * This packet is sent by clients when they want to see a list of available servers
     * @throws ClientTimeoutException  Shouldn't happen as we are sending unreliable flags
     * @throws IOException             If an I/O error occurs
     * @throws InterruptedException    If this function is interrupted during sleep
     */
    private void ListServers() throws IOException, InterruptedException, ClientTimeoutException {
        String addrport = this.handler.GetAddress() + ":" + this.handler.GetPort();
        System.out.println("Client " + addrport + " requested server list");
        for (N64Server server : this.servers.values()) {
            this.handler.SendPacket(new S64Packet("SERVER", server.toByteArray(this.roms.get(server.GetROMHashStr()) != null), PacketFlag.FLAG_UNRELIABLE.GetInt()));
            Thread.sleep(50);
        }
        this.handler.SendPacket(new S64Packet("DONELISTING", null, PacketFlag.FLAG_UNRELIABLE.GetInt()));
        System.out.println("Client " + addrport + " got server list");
    }
    
    /**
     * Handle a S64Packet with the type "HEARTBEAT"
     * This packet is sent by servers to show they are still alive
     */
    private void HandleServerHeartbeat() throws ClientTimeoutException, IOException {
    	String addrport = this.handler.GetAddress() + ":" + this.handler.GetPort();
        N64Server server =  this.servers.get(addrport);
        
        // If the server exists, update the last interaction time
        if (server != null) {
            server.UpdateLastInteractionTime();
        	return;
        }
        
        // Otherwise, send a register command so that the server knows it needs to re-register
        this.handler.SendPacket(new S64Packet("REGISTER", null, PacketFlag.FLAG_UNRELIABLE.GetInt()));
        System.out.println("Client " + addrport + " sent a heartbeat, but isn't registered.");
    }
    
    /**
     * Handle a S64Packet with the type "DOWNLOAD"
     * This packet is sent by clients when they wish to download a ROM
     * @throws ClientTimeoutException  Shouldn't happen as we are sending unreliable flags
     * @throws IOException             If an I/O error occurs
     * @throws InvalidROMException     If a file is found which is not a valid N64 ROM
     */
    private void DownloadROM(byte[] data) throws IOException, ClientTimeoutException, InvalidROMException {
        byte[] hash;
        N64ROM rom;
        String romhashstr;
        File romfile;
        ByteBuffer bb = ByteBuffer.wrap(data);
        String addrport = this.handler.GetAddress() + ":" + this.handler.GetPort();
        System.out.println("Client " + addrport + " wants to download ROM");
        
        // Store the hash
        hash = new byte[data.length];
        for (int i=0; i<data.length; i++)
            hash[i] = bb.get();
        
        // Find the hash in our ROM list
        romhashstr = N64ROM.BytesToHash(hash);
        rom = this.roms.get(romhashstr);
        if (rom == null) {
            System.err.println("    Client requested non-existent ROM");
            this.handler.SendPacket(new S64Packet("DOWNLOAD", null, PacketFlag.FLAG_UNRELIABLE.GetInt()));
            return;
        }
        
        // If the file doesn't exist any longer, delete it from our ROM list
        romfile = new File(rom.GetPath());
        if (!romfile.exists()) {
            System.err.println("    Client requested since-removed ROM");
            this.roms.remove(romhashstr);
            this.handler.SendPacket(new S64Packet("DOWNLOAD", null, PacketFlag.FLAG_UNRELIABLE.GetInt()));
            return;
        }
        
        // If the hash changed, update the hash and stop
        if (Arrays.equals(N64ROM.GetROMHash(romfile), hash) == false) {
            System.err.println("    Client requested since-changed ROM");
            rom = new N64ROM(romfile);
            this.roms.remove(romhashstr);
            this.roms.put(rom.GetHashString(), rom);
            this.handler.SendPacket(new S64Packet("DOWNLOAD", null, PacketFlag.FLAG_UNRELIABLE.GetInt()));
            return;
        }
        
        // Send file data
        this.romtodownload = romfile;
        this.romchunkcount = (int)Math.ceil(((float)rom.GetSize())/FILECHUNKSIZE);
        bb = ByteBuffer.allocate(8);
        bb.putInt(rom.GetSize());
        bb.putInt(FILECHUNKSIZE);
        this.handler.SendPacket(new S64Packet("DOWNLOAD", bb.array(), PacketFlag.FLAG_UNRELIABLE.GetInt()));
    }
    
    /**
     * Handle a S64Packet with the type "FILEDATA"
     * This packet is sent by clients when they want a chunk of the current file to download
     * @throws ClientTimeoutException  Shouldn't happen as we are sending unreliable flags
     * @throws IOException             If an I/O error occurs
     */
    private void SendROMChunk(byte[] data) throws IOException, ClientTimeoutException {
        int readcount;
        int requestedchunk;
        byte[] buffer = new byte[FILECHUNKSIZE];
        BufferedInputStream bis;
        ByteBuffer bb = ByteBuffer.wrap(data);
        
        // If we don't have a ROM to download, notify the client
        if (this.romtodownload == null) {
            this.handler.SendPacket(new S64Packet("FILEDATA", null, PacketFlag.FLAG_UNRELIABLE.GetInt()));
            return;
        }
        
        // Check if the client requested a valid chunk
        requestedchunk = bb.getInt();
        if (requestedchunk < 0 || requestedchunk > this.romchunkcount){
            this.handler.SendPacket(new S64Packet("FILEDATA", null, PacketFlag.FLAG_UNRELIABLE.GetInt()));
            return;
        }
        
        // Transfer the ROM chunk
        bis = new BufferedInputStream(new FileInputStream(this.romtodownload));
        bis.skip(requestedchunk*FILECHUNKSIZE);
        readcount = bis.read(buffer);
        bb = ByteBuffer.allocate(readcount + 4 + 4);
        bb.putInt(requestedchunk);
        bb.putInt(readcount);
        bb.put(buffer, 0, readcount);
        this.handler.SendPacket(new S64Packet("FILEDATA", bb.array(), PacketFlag.FLAG_UNRELIABLE.GetInt()));
        bis.close();
    }
}