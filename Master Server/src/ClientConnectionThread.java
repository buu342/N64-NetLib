import N64.N64ROM;
import N64.N64Server;
import java.util.Hashtable;
import java.util.concurrent.ConcurrentLinkedQueue;

public class ClientConnectionThread implements Runnable {

    Hashtable<String, N64ROM> roms;
    Hashtable<String, N64Server> servers;
    String clientaddr;
    ConcurrentLinkedQueue<byte[]> queue = new ConcurrentLinkedQueue<byte[]>();
    
    ClientConnectionThread(Hashtable<String, N64Server> servers, Hashtable<String, N64ROM> roms, String clientaddr) {
        this.servers = servers;
        this.roms = roms;
        this.clientaddr = clientaddr;
    }
    
    public void SendMessage(byte data[]) {
        byte[] copy = data.clone();
        this.queue.add(copy);
    }
    
    public String GetClientAddress() {
        return this.clientaddr;
    }
    
    public void run() {
        while (true) {
            byte[] data = this.queue.poll();
            if (data != null) {
                System.out.println("Got data");
            }
        }
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
    
    private void RegisterServer(byte[] data) {
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
        
        // Store this server in our list
        // If the server already exists, this will update it instead
        server = new N64Server(servername, maxcount, serveraddress, publicport, romname, romhash);
        this.servers.put(fullserveraddr, server);
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
    
    public void run() {

        try {
            int attempts = 5;
            DataInputStream dis = new DataInputStream(this.clientsocket.getInputStream());
            System.out.println(this.clientsocket);
            while (true) {
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
                } else {
                    System.out.println("Received packet with unknown type '" + pkt.GetType() + "'");
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
    */
    
}