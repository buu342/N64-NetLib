import N64.N64ROM;
import N64.N64Server;
import NetLib.S64Packet;
import NetLib.UDPHandler;
import java.io.IOException;
import java.net.DatagramSocket;
import java.nio.ByteBuffer;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

public class ClientConnectionThread implements Runnable {
    
    ConcurrentHashMap<String, N64ROM> roms;
    ConcurrentHashMap<String, N64Server> servers;
    ConcurrentLinkedQueue<byte[]> msgqueue = new ConcurrentLinkedQueue<byte[]>();
    UDPHandler handler;
    
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
        while (true) {
            
            // Check packets that came from a user (by reading the message queue)
            byte[] data = this.msgqueue.poll();
            if (data != null) {
                try {
                    if (!this.handler.IsS64Packet(data)) {
                        System.err.println("Received data which isn't an S64Packet from " + this.handler.GetAddress() + ":" + this.handler.GetPort());
                        continue;
                    }
                    S64Packet pkt = this.handler.ReceiveS64Packet(data);
                  
                    if (pkt.GetType().equals("REGISTER")) {
                        this.RegisterServer(pkt.GetData());
                        break;
                    } else if (pkt.GetType().equals("LIST")) {
                        this.ListServers();
                        break;
                    } else if (pkt.GetType().equals("DOWNLOAD")) {
                        //this.DownloadROM(pkt.GetData());
                        break;
                    } else {
                        System.out.println("Received packet with unknown type '" + pkt.GetType() + "'");
                        break;
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
    }
    
    private void RegisterServer(byte[] data) {
        int size;
        int maxcount = 0;
        int publicport = 0;
        String servername = "";
        String romname = "";
        String romhashstr;
        byte[] romhash;
        N64Server server;
        ByteBuffer bb = ByteBuffer.wrap(data);
        String addrport = this.handler.GetAddress() + ":" + this.handler.GetPort();
        System.out.println("Client " + addrport + " requested server register");
        
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
        
        // Store this server in our list
        // If the server already exists, this will update it instead
        server = new N64Server(servername, maxcount, this.handler.GetAddress(), publicport, romname, romhash);
        this.servers.put(addrport, server);
        System.out.println("Client " + addrport + " registered successfully");
    }
    
    private void ListServers() throws IOException, InterruptedException {
        
    }
    
    /*
    private void ListServers() throws IOException, InterruptedException {
        DataOutputStream dos = new DataOutputStream(this.clientsocket.getOutputStream());
        int replycount = this.servers.size();
        Queue<Pair<N64Server, S64Packet>> replies = new ConcurrentLinkedQueue<Pair<N64Server, S64Packet>>();
        System.out.println("Client " + this.clientsocket + " requested list of servers.");
        System.out.println("    Sending " + replycount + " servers");
        for (var entry : this.servers.entrySet()) {
            new Thread() {
                N64Server s = entry.getValue();
                Queue<Pair<N64Server, S64Packet>> q = replies;
                public void run() {
                    try {
                        Socket sock = new Socket(s.GetAddress(), s.GetPort());
                        DataInputStream dis = new DataInputStream(sock.getInputStream());
                        DataOutputStream dos = new DataOutputStream(sock.getOutputStream());
                        S64Packet pkt = new S64Packet("PING", null);
                        pkt.WritePacket(dos);
                        pkt = S64Packet.ReadPacket(dis);
                        if (pkt == null)
                            q.add(new Pair<>(s, new S64Packet("PING", null)));
                        else
                            q.add(new Pair<>(s, pkt));
                        dis.close();
                        dos.close();
                        sock.close();
                    } catch (Exception e) {
                        q.add(new Pair<>(s, new S64Packet("PING", null)));
                    }
                }
            }.start();
        }
        
        while (replycount > 0) {
            Pair <N64Server, S64Packet> reply = replies.poll();
            if (reply == null) {
                Thread.sleep(10);
                continue;
            }
            N64Server server = reply.getKey();
            S64Packet pkt = reply.getValue();
            
            if (pkt.GetType().equals("PING")) {
                if (pkt.GetSize() > 0) {
                    byte[] serverbytes = server.toByteArray();
                    byte[] sendbytes = new byte[serverbytes.length + 1 + 1];
                    System.arraycopy(serverbytes, 0, sendbytes, 0, serverbytes.length);
                    if (this.roms.get(server.GetROMHashStr()) != null)
                        sendbytes[serverbytes.length] = 1;
                    else
                        sendbytes[serverbytes.length] = 0;
                    for (int i=0; i<1; i++)
                        sendbytes[serverbytes.length + 1 + i] = pkt.GetData()[i];
                    pkt = new S64Packet("SERVER", sendbytes);
                    pkt.WritePacket(dos);
                } else {
                    String fulladdr = server.GetAddress() + ":" + server.GetPort();
                    if (this.servers.get(fulladdr) == server) {
                        System.out.println("    " + server.GetName() + " at " + fulladdr + " seems to no longer be available.");
                        this.servers.remove(fulladdr);
                    }
                }
                replycount--;
            }
        }
        dos.close();
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