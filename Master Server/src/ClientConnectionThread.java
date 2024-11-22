import N64.N64ROM;
import N64.N64Server;
import NetLib.ClientTimeoutException;
import NetLib.S64Packet;
import NetLib.UDPHandler;
import java.io.IOException;
import java.net.DatagramSocket;
import java.nio.ByteBuffer;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

public class ClientConnectionThread extends Thread {

    UDPHandler handler;
    ConcurrentHashMap<String, N64ROM> roms;
    ConcurrentHashMap<String, N64Server> servers;
    ConcurrentLinkedQueue<byte[]> msgqueue = new ConcurrentLinkedQueue<byte[]>();
    
    ClientConnectionThread(ConcurrentHashMap<String, N64Server> servers, ConcurrentHashMap<String, N64ROM> roms, DatagramSocket socket, String addr, int port) {
        this.servers = servers;
        this.roms = roms;
        this.handler = new UDPHandler(socket, addr, port);
    }
    
    public void SendMessage(byte data[], int size) {
        byte[] copy = new byte[size];
        System.arraycopy(data, 0, copy, 0, size);
        this.msgqueue.add(copy);
    }
    
    public void run() {
        try {
            while (true) {
                // Check packets that came from a user (by reading the message queue)
                byte[] data = this.msgqueue.poll();
                if (data != null) {
                    if (!this.handler.IsS64Packet(data)) {
                        System.err.println("Received data which isn't an S64Packet from " + this.handler.GetAddress() + ":" + this.handler.GetPort());
                        continue;
                    }
                    S64Packet pkt = this.handler.ReadS64Packet(data);
                  
                    if (pkt.GetType().equals("REGISTER")) {
                        this.RegisterServer(pkt.GetData());
                        break;
                    } else if (pkt.GetType().equals("LIST")) {
                        this.ListServers();
                        break;
                    } else if (pkt.GetType().equals("DOWNLOAD")) {
                        //this.DownloadROM(pkt.GetData());
                        break;
                    } else if (pkt.GetType().equals("ACK")) {
                        continue;
                    } else {
                        System.out.println("Received packet with unknown type '" + pkt.GetType() + "'");
                        break;
                    }
                } else {
                    Thread.sleep(10);
                }
            }
        //} catch (ClientTimeoutException e) {
        //    System.out.println(e);
        //    return;
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    private void RegisterServer(byte[] data) throws IOException {
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
        
        // Send an ack, and finish
        this.handler.SendPacket(new S64Packet("ACK", null));
        System.out.println("Client " + addrport + " registered successfully");
    }
    
    private void ListServers() throws IOException, InterruptedException {
        String addrport = this.handler.GetAddress() + ":" + this.handler.GetPort();
        System.out.println("Client " + addrport + " requested server list");
        for (N64Server server : this.servers.values()) {
            this.handler.SendPacket(new S64Packet("SERVER", server.toByteArray(this.roms.get(server.GetROMHashStr()) != null)));
            Thread.sleep(10);
        }
        this.handler.SendPacket(new S64Packet("DONELISTING", null));
        System.out.println("Client " + addrport + " got server list");
    }
    
    /*    
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
    */
}