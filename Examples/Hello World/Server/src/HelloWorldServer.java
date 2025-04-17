import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map.Entry;
import com.dosse.upnp.UPnP;
import NetLib.S64Packet;

public class HelloWorldServer {

    // Constants
    private static final String MASTER_DEFAULTADDRESS = "127.0.0.1";
    private static final int    MASTER_DEFAULTPORT = 6464;
    
    // Server settings
    private static int port = 6460;
    private static boolean useupnp = true;
    private static boolean register = true;
    private static String servername = "";
    private static int maxplayers = 32;
    private static String masteraddress = MASTER_DEFAULTADDRESS + ":" + MASTER_DEFAULTPORT;
    private static String romname = "";
    private static byte[] romhash;
    
    // Database
    private static Hashtable<String, ClientConnectionThread> connectiontable = new Hashtable<>();

    /**
     * Program entrypoint
     * @param args  A list of arguments passed to the program
     */
    public static void main(String args[]) throws Exception {
        MasterConnectionThread master = null;
        DatagramSocket ds = null;
        int masterport = 0;
        byte[] data = new byte[S64Packet.PACKET_MAXSIZE];

        // Check for program arguments
        ReadArguments(args);
        
        // Read the master server address
        if (register) {
            int cpos = masteraddress.indexOf(":");
            masterport = Integer.parseInt(masteraddress.substring(cpos+1 , masteraddress.length()));
            masteraddress = masteraddress.substring(0 , cpos);
            
            // If the name or ROM arguments are invalid, exit
            if (servername.equals("") || romname.equals("")) {
                ShowHelp();
                System.exit(1);
            }
            
            // Try to open the ROM
            romname = ValidateN64ROM(romname);
        }
        
        // Try to UPnP the port
        if (useupnp) {
            Runtime.getRuntime().addShutdownHook(
                new Thread("App Shutdown Hook") {
                    public void run() { 
                        UPnP.closePortTCP(port);
                        System.out.println("UPnP port closed");
                    }
                }
            );
            System.out.println("Attempting UPnP port forwarding...");
            if (UPnP.isUPnPAvailable()) {
                if (UPnP.isMappedTCP(port)) {
                    System.out.println("UPnP port is already mapped");
                } else if (UPnP.openPortTCP(port)) {
                    System.out.println("UPnP enabled");
                } else {
                    System.out.println("UPnP failed");
                }
            } else {
                System.out.println("UPnP is not available");
            }
        }
        ds = new DatagramSocket(port);
        
        // Try to connect to the master server and register ourselves
        if (register) {
            System.out.println("Registering to master server");
            try {
                master = new MasterConnectionThread(ds, masteraddress, masterport);
                master.start();
            } catch (Exception e) {
                System.err.println("Unable to register to master server");
                e.printStackTrace();
            }
        }
        
        // Allow clients to connect, and pass messages over to them
        System.out.println("Server is ready to accept players.");
        while (true) {
            DatagramPacket udppkt;
            try {
                String clientaddr;
                ClientConnectionThread t;
                Iterator<Entry<String, ClientConnectionThread>> it;
                udppkt = new DatagramPacket(data, data.length);
                ds.receive(udppkt);
                clientaddr = udppkt.getAddress().getHostAddress() + ":" + udppkt.getPort();
                
                // Check first if they're packets from the master server
                if (clientaddr.equals(masteraddress + ":" + masterport)) {
                    if (register)
                        master.SendMessage(data, udppkt.getLength());
                    continue;
                }
                
                // Clean up the connection table of dead clients
                it = connectiontable.entrySet().iterator();
                while (it.hasNext()) {
                    Entry<String, ClientConnectionThread> entry = it.next();
                    if (!entry.getValue().isAlive())
                        it.remove();
                }
                
                // If they aren't, handle a client packet
                t = connectiontable.get(clientaddr);
                if (t == null) {
                    t = new ClientConnectionThread(ds, udppkt.getAddress().getHostAddress(), udppkt.getPort());
                    t.start();
                    connectiontable.put(clientaddr, t);
                }
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
                case "-name":
                    if (i+1 >= args.length) {
                        System.err.println("Missing argument for name command");
                        ShowHelp();
                        System.exit(1);
                    }
                    servername = args[++i];
                    break;
                case "-rom":
                    if (i+1 >= args.length) {
                        System.err.println("Missing argument for rom command");
                        ShowHelp();
                        System.exit(1);
                    }
                    romname = args[++i];
                    break;
                case "-port":
                    if (i+1 >= args.length) {
                        System.err.println("Missing argument for port command");
                        ShowHelp();
                        System.exit(1);
                    }
                    port = Integer.parseInt(args[++i]);
                    break;
                case "-noregister":
                    register = false;
                    break;
                case "-noupnp":
                    useupnp = false;
                    break;
                case "-master":
                    if (i+1 >= args.length) {
                        System.err.println("Missing argument for master command");
                        ShowHelp();
                        System.exit(1);
                    }
                    masteraddress = args[++i];
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
        System.out.println("    -name <Server Name>\t\t(REQUIRED) Server name");
        System.out.println("    -rom <Path/To/File.n64>\t(REQUIRED) ROM to use");
        System.out.println("    -port <Port Number>\t\tServer port (default '6460')");
        System.out.println("    -noregister\t\t\tDo not register to the master server");
        System.out.println("    -noupnp\t\t\tDo not use UPNP to open the port");
        System.out.println("    -master <Address:Port>\tMaster server connection (default '" + MASTER_DEFAULTADDRESS + ":" + MASTER_DEFAULTPORT + "')");
        System.out.println("    -help\t\t\tDisplay this");   
    }

    /**
     * Validate the given file to ensure it's an N64 ROM
     * @param rompath  The path to the file
     * @return  The name of the ROM file, or ""
     */
    private static String ValidateN64ROM(String rompath) {    
        try {
            byte[] rombytes;
            File rom = new File(rompath);
            if(!rom.exists() || rom.isDirectory()) {
                System.err.println(rompath + " is not a valid file!");
                System.exit(1);
            }
            
            // Validate the ROM header. No reason to support v64 ROMs.
            rombytes = Files.readAllBytes(rom.toPath());
            if (!(rombytes[0] == (byte)0x80 && rombytes[1] == (byte)0x37 && rombytes[2] == (byte)0x12 && rombytes[3] == (byte)0x40)) {
                System.err.println(rompath + " is not a valid ROM file!");
                System.exit(1);
            }
            
            // Generate the hash
            romhash = GenerateHashFromROM(rompath, rombytes);
            return rom.getName();
        } catch (Exception e) {
            System.err.println("Unable to read file "+rompath);
            e.printStackTrace();
            System.exit(1);
        }
        return "";
    }

    /**
     * Creates the SHA256 hash representation of the ROM file
     * @param rompath   The path to the file to get the hash of
     * @param rombytes  The raw bytes of the file
     * @return  The SHA256 hash representation of the ROM file
     */
    private static byte[] GenerateHashFromROM(String rompath, byte[] rombytes) throws Exception {
        int count;
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        BufferedInputStream bis = new BufferedInputStream(new FileInputStream(rompath));
        
        // Generate the sha256 hash, and assign the ROM size
        while ((count = bis.read(rombytes)) > 0)
            digest.update(rombytes, 0, count);
        bis.close();
        return digest.digest();
    }

    /**
     * Convert this server's information into a byte array, to send to the Master Server
     * @return  The byte array representation of the server, or null
     */
    public static byte[] ToByteArray_Master() {
        try {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            bytes.write(ByteBuffer.allocate(4).putInt(port).array());
            bytes.write(ByteBuffer.allocate(4).putInt(romname.length()).array());
            bytes.write(romname.getBytes());
            bytes.write(ByteBuffer.allocate(4).putInt(romhash.length).array());
            bytes.write(romhash);
            return bytes.toByteArray();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * Convert this server's information into a byte array, to send to a client
     * @param identifier  The server identifier to echo back to the client
     * @return  The byte array representation of the server, or null
     */
    public static byte[] ToByteArray_Client(String identifier) {
        try {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            bytes.write(ByteBuffer.allocate(4).putInt(identifier.length()).array());
            bytes.write(identifier.getBytes());
            bytes.write(ByteBuffer.allocate(4).putInt(servername.length()).array());
            bytes.write(servername.getBytes());
            bytes.write(ByteBuffer.allocate(4).putInt(0).array()); // Always zero players
            bytes.write(ByteBuffer.allocate(4).putInt(maxplayers).array());
            return bytes.toByteArray();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
}