import N64.N64ROM;
import N64.N64Server;
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
    
    private static final int FILECHUNKSIZE = 2048;

    private File romtodownload;
    private int romchunkcount;
    private UDPHandler handler;
    private ConcurrentHashMap<String, N64ROM> roms;
    private ConcurrentHashMap<String, N64Server> servers;
    private ConcurrentLinkedQueue<byte[]> msgqueue = new ConcurrentLinkedQueue<byte[]>();
    
    ClientConnectionThread(ConcurrentHashMap<String, N64Server> servers, ConcurrentHashMap<String, N64ROM> roms, DatagramSocket socket, String addr, int port) {
        this.servers = servers;
        this.roms = roms;
        this.handler = new UDPHandler(socket, addr, port);
        this.romtodownload = null;
        this.romchunkcount = 0;
    }
    
    public void SendMessage(byte data[], int size) {
        byte[] copy = new byte[size];
        System.arraycopy(data, 0, copy, 0, size);
        this.msgqueue.add(copy);
    }
    
    public void run() {
        try {
            while (!Thread.currentThread().isInterrupted()) {
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
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    private void RegisterServer(byte[] data) throws IOException, ClientTimeoutException {
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
    
    private void HandleServerHeartbeat() throws IOException, ClientTimeoutException {
        N64Server server =  this.servers.get(this.handler.GetAddress() + ":" + this.handler.GetPort());
        if (server != null)
            server.UpdateLastInteractionTime();
    }
    
    private void DownloadROM(byte[] data) throws Exception, IOException {
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
    
    private void SendROMChunk(byte[] data) throws IOException, ClientTimeoutException, InterruptedException {
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
        
        // Transfer the ROM
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