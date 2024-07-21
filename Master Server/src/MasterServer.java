import java.net.ServerSocket;
import java.net.Socket;
import java.io.File;
import java.io.IOException;
import java.util.Hashtable;
import NetLib.N64Server;
import NetLib.N64ROM;

public class MasterServer {
	
	private static final int DEFAULTPORT = 6464;
	
	private static Hashtable<String, N64ROM> romtable = new Hashtable<>();
	private static Hashtable<String, N64Server> servertable = new Hashtable<>();

    private static String romdir = "roms/";
	
	public static void main(String args[]) throws IOException {
		boolean isrunning = true;
		ServerSocket ss = null;
		System.out.println("Starting N64NetLib Master Server.");

        // Open the ROM directory and get all the ROMs inside
		ReadROMs();
		
		// Try to open the port
        try {
        	ss = new ServerSocket(DEFAULTPORT);
        } catch (IOException e) {
            System.err.println("Failed to open port " + Integer.toString(DEFAULTPORT) + ".");
            System.exit(1);
        }
        System.out.println("Successfully opened socket at port " + Integer.toString(DEFAULTPORT) + ".");
        
        // Try to connect a client
        while (isrunning) {
            Socket s = null;
            ClientConnectionThread t;
	        try {
	            s = ss.accept();
	            t = new ClientConnectionThread(servertable, romtable, s);
	            new Thread(t).start();
	        } catch (IOException e) {
	            System.err.println("Error during client connection.");
	            e.printStackTrace();
	        }
        }
		
		// End
		ss.close();
	}
	
	private static void ReadROMs()
	{
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
	
	public static void ValidateROM(String name)
	{
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