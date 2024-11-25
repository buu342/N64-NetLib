import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.io.File;
import java.io.IOException;
import java.util.Hashtable;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;

import N64.N64ROM;
import N64.N64Server;
import NetLib.S64Packet;

public class MasterServer {
    
    private static final int TIME_KEEPSERVERS = 1000*60*10;
    private static final int DEFAULTPORT = 6464;
    
    private static int port = DEFAULTPORT;
    private static ConcurrentHashMap<String, N64ROM> romtable = new ConcurrentHashMap<>();
    private static ConcurrentHashMap<String, N64Server> servertable = new ConcurrentHashMap<>();
    private static Hashtable<String, ClientConnectionThread> connectiontable = new Hashtable<>();

    private static String romdir = "roms/";
    
    public static void main(String args[]) throws IOException {
        byte[] data = new byte[S64Packet.PACKET_MAXSIZE];
        DatagramSocket ds = null;
        
        // Check for optional arguments
        ReadArguments(args);
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
                
                // Receive packets
                udppkt = new DatagramPacket(data, data.length);
                ds.receive(udppkt);
                clientaddr = udppkt.getAddress().getHostAddress() + ":" + udppkt.getPort();
                t = connectiontable.get(clientaddr);
                
                // Check for dead servers
                for (Entry<String, N64Server> entry : servertable.entrySet()) {
                    if (System.currentTimeMillis() - entry.getValue().GetLastInteractionTime() > TIME_KEEPSERVERS) {
                        System.err.println("No interactions from "+entry.getKey()+". Removing from registry.");
                        servertable.remove(entry.getKey());
                    }
                }
                
                // Create a thread for this client if it doesn't exist
                if (t == null || !t.isAlive()) {
                    t = new ClientConnectionThread(servertable, romtable, ds, udppkt.getAddress().getHostAddress(), udppkt.getPort());
                    new Thread(t).start();
                    connectiontable.put(clientaddr, t);
                }
                
                // Send the packet to this client thread
                t.SendMessage(data, udppkt.getLength());
                
                // Clean up the connection table of dead clients
                for (Entry<String, ClientConnectionThread> entry : connectiontable.entrySet())
                    if (!entry.getValue().isAlive())
                        connectiontable.remove(entry.getKey());
            } catch (Exception e) {
                System.err.println("Error during client connection.");
                e.printStackTrace();
            }
        }
    }
    
    private static void ReadArguments(String args[]) {
        for (int i=0; i<args.length; i++) {
            switch (args[i]) {
                case "-port":
                    if (i+1 >= args.length) {
                        System.err.println("Missing argument for port command");
                        ShowHelp();
                        System.exit(1);
                    }
                    port = Integer.parseInt(args[++i]);
                    break;
                case "-dir":
                    if (i+1 >= args.length) {
                        System.err.println("Missing argument for port command");
                        ShowHelp();
                        System.exit(1);
                    }
                    romdir = args[++i];
                    break;
                case "-help":
                default:
                    ShowHelp();
                    System.exit(0);
            }
        }
    }
    
    private static void ShowHelp() {
        System.out.println("Program arguments:");
        System.out.println("    -help\t\tDisplay this");
        System.out.println("    -port <Number>\tServer port (default '6464')");
        System.out.println("    -dir <Path/To/Dir>\tROM Directory (default 'roms')");
    }
    
    private static void ReadROMs() {
        File[] files;
        File folder = new File(romdir + "/");
        
        // Create the folder if it doesn't exist
        if (!folder.exists())
            folder.mkdirs();
        
        // Loop through all ROMs and register them in the ROM table
        System.out.println("Checking ROMs folder.");
        files = folder.listFiles();
        for (int i=0; i<files.length; i++) {
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
    
    public static void FindROMWithName(String name) {
        File file = new File(romdir + "/" + name);
        if (file.exists()) {
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