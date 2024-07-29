import java.net.ServerSocket;
import java.net.Socket;
import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.regex.Pattern;
import java.util.regex.Matcher;

public class TicTacToeServer {
    
    private static final String MASTER_DEFAULTADDRESS = "127.0.0.1";
    private static final int    MASTER_DEFAULTPORT = 6464;
    
    private static int port = 0;
    private static String servername = "Tic Tac Toe Server";
    private static int maxplayers = 2;
    private static String romname;
    private static byte[] romhash;
    
    public static void main(String args[]) throws Exception {
        ServerSocket ss = null;
        boolean isrunning = true;
        String masteraddress = MASTER_DEFAULTADDRESS;
        int masterport = MASTER_DEFAULTPORT;
        ClientConnectionThread t;

        // Validate program arguments
        if (args.length < 3 || args.length > 4) {
            System.out.println("Arguments: <Port> <Path/To/Rom.n64> <Server Name> [(Optional) Master Server Address:Port]");
            System.exit(0);
        }
        
        // Grab program arguments
        port = Integer.parseInt(args[0]);
        servername = args[2];
        
        // Try to open the ROM
        romname = ValidateN64ROM(args[1]);
        
        // Read the master server address
        if (args.length == 4) {
            int cpos;
            Pattern compiledPattern = Pattern.compile("\\d{1,3}(?:\\.\\d{1,3}){3}(?::\\d{1,5})?");
            Matcher matcher = compiledPattern.matcher(args[3]);
            masteraddress = matcher.group();
            cpos = masteraddress.indexOf(":");
            if (cpos > 0)
                port = Integer.getInteger(masteraddress.substring(cpos+1 , masteraddress.length()));
            masteraddress = masteraddress.substring(0 , cpos);
        }
            
        // Try to connect to the master server and register ourselves
        System.out.println("Registering to master server");
        try {
            System.out.println(masteraddress);
            System.out.println(masterport);
            Socket smaster = new Socket(masteraddress, masterport);
            DataOutputStream dos = new DataOutputStream(smaster.getOutputStream());
            byte[] serverbytes = ToByteArray();
            byte[] header = {'N', '6', '4', 'P', 'K', 'T'};
            byte[] packetype = {'R', 'E', 'G', 'I', 'S', 'T', 'E', 'R'};
            dos.write(header);
            // TODO: Send the packet version
            dos.writeInt(serverbytes.length + packetype.length);
            dos.write(packetype);
            dos.write(serverbytes);
            dos.flush();
            dos.close();
            smaster.close();
        } catch (Exception e) {
            System.err.println("Unable to register to master server");
            e.printStackTrace();
        }
        System.out.println("Done. Comitting seppuku.");
            
        // Try to open the server port
        try {
            ss = new ServerSocket(port);
        } catch (Exception e) {
            System.err.println("Failed to open server port "+port+".");
            e.printStackTrace();
            System.exit(1);
        }
        
        // Allow clients to connect
        while (isrunning) {
            Socket s = null;
            try {
                s = ss.accept();
                t = new ClientConnectionThread(s);
                new Thread(t).start();
            } catch (Exception e) {
                System.err.println("Error during client connection.");
                e.printStackTrace();
            }
        }
        
        // End
        ss.close();
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
    
    private static byte[] ToByteArray()    {
        try {
            ByteArrayOutputStream bytes    = new ByteArrayOutputStream();
            bytes.write(ByteBuffer.allocate(4).putInt(servername.length()).array());
            bytes.write(servername.getBytes());
            bytes.write(ByteBuffer.allocate(4).putInt(maxplayers).array());
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
}