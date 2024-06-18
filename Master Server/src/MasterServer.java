import java.net.ServerSocket;
import java.net.Socket;
import java.io.IOException;
import java.util.Hashtable;
import NetLib.N64Sever;


public class MasterServer {
	
	private static final int DEFAULTPORT = 6464;
	
	private Hashtable<String, N64Sever> servertable = new Hashtable<>();
	
	public static void main(String args[]) throws IOException {
		boolean isrunning = true;
		ServerSocket ss = null;
		Socket s = null;
		
		// Try to open the port
        try {
        	ss = new ServerSocket(DEFAULTPORT);
        } catch (IOException e) {
            System.err.println("Failed to open port " + Integer.toString(DEFAULTPORT) + ".");
            System.exit(1);
        }
        System.out.println("Successfully opened socket at port" + Integer.toString(DEFAULTPORT) + ".");
        
        // Try to connect a client
        while (isrunning) {
	        try {
	            s = ss.accept();
	        } catch (IOException e) {
	            System.err.println("Client attempted to connect and failed.");
	        }
			System.out.println("Client connected");
        }
		
		// End
		ss.close();
	}
}