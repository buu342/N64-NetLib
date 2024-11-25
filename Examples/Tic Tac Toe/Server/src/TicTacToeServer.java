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
import com.dosse.upnp.UPnP;

import NetLib.S64Packet;

public class TicTacToeServer {
    
    private static final String MASTER_DEFAULTADDRESS = "127.0.0.1";
    private static final int    MASTER_DEFAULTPORT = 6464;
    
    private static int port = 6460;
    private static boolean useupnp = true;
    private static boolean register = true;
    private static String servername = "";
    private static int maxplayers = 2;
    private static String masteraddress = MASTER_DEFAULTADDRESS + ":" + MASTER_DEFAULTPORT;
    private static String romname = "";
    private static byte[] romhash;
    private static TicTacToe.Game game;
    private static Hashtable<String, ClientConnectionThread> connectiontable = new Hashtable<>();
    
    public static void main(String args[]) throws Exception {
        MasterConnectionThread master = null;
        DatagramSocket ds = null;
        int masterport = 0;
        byte[] data = new byte[S64Packet.PACKET_MAXSIZE];

        // Check for program arguments
        ReadArguments(args);
        
        // If the name or rom arguments are invalid, exit
        if (servername.equals("") || romname.equals("")) {
            ShowHelp();
            System.exit(1);
        }
        
        // Try to open the ROM
        romname = ValidateN64ROM(romname);
        
        // Read the master server address
        if (register) {
            int cpos = masteraddress.indexOf(":");
            masterport = Integer.parseInt(masteraddress.substring(cpos+1 , masteraddress.length()));
            masteraddress = masteraddress.substring(0 , cpos);
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
                new Thread(master).start();
            } catch (Exception e) {
                System.err.println("Unable to register to master server");
                e.printStackTrace();
            }
        }
        
        // Begin the game
        game = new TicTacToe.Game();
        new Thread(game).start();
        
        // Allow clients to connect, and pass messages over to them
        System.out.println("Server is ready to accept players.");
        while (true) {
            DatagramPacket udppkt;
            try {
                String clientaddr;
                ClientConnectionThread t;
                udppkt = new DatagramPacket(data, data.length);
                ds.receive(udppkt);
                clientaddr = udppkt.getAddress().getHostAddress() + ":" + udppkt.getPort();
                
                // Check first if they're packets from the master server
                if (clientaddr.equals(masteraddress + ":" + masterport)) {
                    if (register)
                        master.SendMessage(data, udppkt.getLength());
                    continue;
                }
                
                // If they aren't, handle a client packet
                t = connectiontable.get(clientaddr);
                if (t == null) {
                    t = new ClientConnectionThread(ds, udppkt.getAddress().getHostAddress(), udppkt.getPort(), game);
                    new Thread(t).start();
                    connectiontable.put(clientaddr, t);
                }
                t.SendMessage(data, udppkt.getLength());
            } catch (Exception e) {
                System.err.println("Error during client connection.");
                e.printStackTrace();
            }
        }
    }
    
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
    
    public static byte[] ToByteArray_Client(String identifier) {
        try {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            bytes.write(ByteBuffer.allocate(4).putInt(identifier.length()).array());
            bytes.write(identifier.getBytes());
            bytes.write(ByteBuffer.allocate(4).putInt(servername.length()).array());
            bytes.write(servername.getBytes());
            bytes.write(ByteBuffer.allocate(4).putInt(game.PlayerCount()).array());
            bytes.write(ByteBuffer.allocate(4).putInt(maxplayers).array());
            return bytes.toByteArray();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
}