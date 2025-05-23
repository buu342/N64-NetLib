import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.io.File;
import java.io.IOException;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map.Entry;
import java.util.concurrent.ConcurrentHashMap;
import com.dosse.upnp.UPnP;
import N64.N64ROM;
import N64.N64Server;
import NetLib.S64Packet;

public class MasterServer {
    
	// Constants
    private static final int TIME_KEEPSERVERS = 1000*60*10;
    private static final int DEFAULTPORT = 6464;
    
    // Database structs
    private static ConcurrentHashMap<String, N64ROM> romtable = new ConcurrentHashMap<>();
    private static ConcurrentHashMap<String, N64Server> servertable = new ConcurrentHashMap<>();
    private static Hashtable<String, ClientConnectionThread> connectiontable = new Hashtable<>();

    // Default globals
    private static int port = DEFAULTPORT;
    private static boolean useupnp = true;
    private static String romdir = "roms/";
    
    /**
     * Program entrypoint
     * @param args  A list of arguments passed to the program
     */
    public static void main(String args[])  {
        byte[] data = new byte[S64Packet.PACKET_MAXSIZE];
        DatagramSocket ds = null;
        
        // Check for optional arguments
        ReadArguments(args);
        System.out.println("Starting N64NetLib Master Server.");

        // Open the ROM directory and get all the ROMs inside
        ReadROMs();
        
        // Try to open the port
        try {
            if (useupnp) {
                Runtime.getRuntime().addShutdownHook(
                    new Thread("App Shutdown Hook") {
                        public void run() { 
                            UPnP.closePortTCP(port);
                            UPnP.closePortUDP(port);
                            System.out.println("UPnP port closed");
                        }
                    }
                );
                System.out.println("Attempting UPnP port forwarding...");
                if (UPnP.isUPnPAvailable()) {
                    if (UPnP.isMappedUDP(port) && UPnP.isMappedTCP(port)) {
                        System.out.println("UPnP port is already mapped");
                    } else if (UPnP.openPortUDP(port) && UPnP.openPortTCP(port)) {
                        System.out.println("UPnP enabled");
                    } else {
                        System.out.println("UPnP failed");
                    }
                } else {
                    System.out.println("UPnP is not available");
                }
            }
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
                Iterator<Entry<String, N64Server>> it_s;
                Iterator<Entry<String, ClientConnectionThread>> it_c;
                
                // Receive packets
                udppkt = new DatagramPacket(data, data.length);
                ds.receive(udppkt);
                clientaddr = udppkt.getAddress().getHostAddress() + ":" + udppkt.getPort();
                
                // Check for dead servers
                it_s = servertable.entrySet().iterator();
                while (it_s.hasNext()) {
                    Entry<String, N64Server> entry = it_s.next();
                    if (System.currentTimeMillis() - entry.getValue().GetLastInteractionTime() > TIME_KEEPSERVERS) {
                        it_s.remove();
                        System.err.println("No interactions from "+entry.getKey()+". Removing from registry.");
                    }
                }
                
                // Clean up the connection table of dead clients
                it_c = connectiontable.entrySet().iterator();
                while (it_c.hasNext()) {
                    Entry<String, ClientConnectionThread> entry = it_c.next();
                    if (!entry.getValue().isAlive())
                        it_c.remove();
                }
                
                // Create a thread for this client if it doesn't exist
                t = connectiontable.get(clientaddr);
                if (t == null) {
                    t = new ClientConnectionThread(servertable, romtable, ds, udppkt.getAddress().getHostAddress(), udppkt.getPort());
                    t.start();
                    connectiontable.put(clientaddr, t);
                }
                
                // Send the packet to this client thread
                t.SendMessage(data, udppkt.getLength());
            } catch (Exception e) {
                System.err.println("Error during client connection.");
                e.printStackTrace();
            }
        }
    }
    
    /**
     * Parse command line arguments passed to the program
     * @param args  A list of arguments passed to the program
     */
    private static void ReadArguments(String args[]) {
        for (int i=0; i<args.length; i++) {
            switch (args[i]) {
                case "-noupnp":
                    useupnp = false;
                    break;
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
    
    /**
     * Show the help text
     */
    private static void ShowHelp() {
        System.out.println("Program arguments:");
        System.out.println("    -help\t\tDisplay this");
        System.out.println("    -noupnp\t\tDo not use UPNP to open the port");
        System.out.println("    -port <Number>\tServer port (default '6464')");
        System.out.println("    -dir <Path/To/Dir>\tROM Directory (default 'roms')");
    }
    
    /**
     * Read the ROMs folder
     */
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
    
    /**
     * Finds a ROM with a given name and puts it in our ROM table
     * @param name  The name of the ROM
     */
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