import java.net.ServerSocket;
import java.net.Socket;
import java.io.IOException;
import java.util.Hashtable;
import NetLib.N64Server;

public class MasterServer {
	
	private static final int DEFAULTPORT = 6464;
	
	private static Hashtable<String, N64Server> servertable = new Hashtable<>();

    private static String romdir = "roms/";
	
	public static void main(String args[]) throws IOException {
		boolean isrunning = true;
		ServerSocket ss = null;

        // Open the ROM directory and get all the ROMs inside
		
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
	            t = new ClientConnectionThread(servertable, s);
	            new Thread(t).start();
	        } catch (IOException e) {
	            System.err.println("Error during client connection.");
	            e.printStackTrace();
	        }
        }
		
		// End
		ss.close();
	}
}