import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.io.File;
import java.io.IOException;
import java.util.Hashtable;
import N64.N64ROM;
import N64.N64Server;
import NetLib.S64Packet;

public class MasterServer {
    
    private static final int DEFAULTPORT = 6464;
    
    private static Hashtable<String, N64ROM> romtable = new Hashtable<>();
    private static Hashtable<String, N64Server> servertable = new Hashtable<>();
    private static Hashtable<String, ClientConnectionThread> connectiontable = new Hashtable<>();

    private static String romdir = "roms/";
    
    @SuppressWarnings("resource")
    public static void main(String args[]) throws IOException {
        byte[] data = new byte[S64Packet.PACKET_MAXSIZE];
        DatagramSocket ds = null;
        int port = DEFAULTPORT;
        
        // Ensure we have enough arguments
        if (args.length == 1 ) {
            port = Integer.getInteger(args[0]);
        }
        System.out.println("Starting N64NetLib Master Server.");

        // Open the ROM directory and get all the ROMs inside
        ReadROMs();
        
        // Try to open the port
        try {
            ds = new DatagramSocket(port);
        } catch (IOException e) {
            System.err.println("Failed to open port " + Integer.toString(port) + ".");
            System.exit(1);
        }
        System.out.println("Successfully opened socket at port " + Integer.toString(port) + ".");
        System.out.println();

        // Pass messages over to clients
        while (true) {
            DatagramPacket udppkt;
            try {
                String clientaddr;
                ClientConnectionThread t;
                udppkt = new DatagramPacket(data, data.length);
                ds.receive(udppkt);
                clientaddr = udppkt.getAddress().toString() + ":" + udppkt.getPort();
                t = connectiontable.get(clientaddr);
                if (t != null) {
                    t.SendMessage(data);
                } else {
                    t = new ClientConnectionThread(servertable, romtable, clientaddr);
                    new Thread(t).start();
                    connectiontable.put(clientaddr, t);
                }
            } catch (Exception e) {
                System.err.println("Error during client connection.");
                e.printStackTrace();
            }
        }
    }
    
    private static void ReadROMs() {
        File[] files;
        File folder = new File(romdir);
        
        // Create the folder if it doesn't exist
        if (!folder.exists())
            folder.mkdirs();
        
        // Loop through all ROMs and register them in the ROM table
        System.out.println("Checking ROMs folder.");
        files = folder.listFiles();
        for (int i=0; i<files.length; i++)
        {
            try {
                N64ROM rom = new N64ROM(files[i]);
                romtable.put(rom.GetHashString(), rom);
                System.out.println("Registered '"+files[i].getName()+"'.");
            } catch (Exception e) {
                System.err.println("Invalid ROM file '"+files[i].getName()+"'.");
                e.printStackTrace();
            }
        }
        System.out.println("Done parsing ROMs folder. "+romtable.size()+" ROMs found.");
    }
    
    public static void ValidateROM(String name) {
        File file = new File(romdir + name);
        if (file.exists())
        {
            try {
                N64ROM rom = new N64ROM(file);
                romtable.put(rom.GetHashString(), rom);
                return;
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
}