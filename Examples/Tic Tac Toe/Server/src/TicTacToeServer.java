import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.io.RandomAccessFile;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.Base64;

public class TicTacToeServer {
	
	private static final String MASTER_DEFAULTADDRESS = "127.0.0.1";
	private static final int    MASTER_DEFAULTPORT = 6464;
	
	private static int port = 0;
	private static String servername = "Tic Tac Toe Server";
	private static int playercount = 2;
	private static String romname;
	private static String romhash;
	
	public static void main(String args[]) throws Exception {
		ServerSocket ss = null;
		boolean isrunning = true;

        // Validate program arguments
		if (args.length < 3) {
			System.out.println("Arguments: <Port> <Path/To/Rom.n64> <Server Name> [(Optional) Master Server Address]");
            System.exit(0);
		}
		
		// Grab program arguments
		port = Integer.parseInt(args[0]);
		servername = args[2];
		
		// Try to open the ROM
		romname = ValidateN64ROM(args[1]);
		
		// Read the master server address
		if (args.length == 4) {
			
		}
	        
        // Try to connect to the master server and register ourselves
			
		// Try to open the server port
        try {
        	ss = new ServerSocket(port);
        } catch (IOException e) {
            System.err.println(e);
            System.exit(1);
        }
        
        // Try to connect a client
        while (isrunning) {
            Socket s = null;
	        try {
	            s = ss.accept();
	        } catch (IOException e) {
	            System.err.println("Error during client connection.");
	            System.err.println(e);
	        }
        }
		
		// End
		ss.close();
	}
	
	private static String ValidateN64ROM(String rompath)
	{
		try {
			byte[] rombytes;
			File rom = new File(rompath);
			if(!rom.exists() || rom.isDirectory()) {
	            System.err.println(rompath + " is not a valid file!");
	            System.exit(1);
			}
			
			// Validate the ROM header. No reason to support v64 ROMs.
			rombytes = Files.readAllBytes(rom.toPath());
			if (!(rombytes[0] == 0x80 && rombytes[1] == 0x37 && rombytes[2] == 0x12 && rombytes[3] == 0x40)) {
	            System.err.println(rompath + " is not a valid ROM file!");
	            System.exit(1);
			}
			
			// Generate the hash
			romhash = GenerateHashFromROM(rompath, rombytes);
			return rom.getName();
		} catch (Exception e) {
			System.err.println("Unable to read file "+rompath);
            System.err.println(e);
            System.exit(1);
		}
		return "";
	}
	
	private static String GenerateHashFromROM(String rompath, byte[] rombytes) throws Exception
	{
		byte[] hash;
	    int count;
		MessageDigest digest = MessageDigest.getInstance("SHA-256");
	    BufferedInputStream bis = new BufferedInputStream(new FileInputStream(rompath));
	    
		// Generate the sha256 hash, and assign the ROM size
	    while ((count = bis.read(rombytes)) > 0) {
	        digest.update(rombytes, 0, count);
	    }
	    bis.close();
	    hash = digest.digest();
	    
	    // Assign the hash
	    return new String(Base64.getEncoder().encode(hash));
	}
}