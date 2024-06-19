import java.net.ServerSocket;
import java.net.Socket;
import java.io.IOException;
import java.util.Hashtable;
import NetLib.N64Server;


public class MasterServer {
	
	private static final int DEFAULTPORT = 6464;
	
	private static Hashtable<String, N64Server> servertable = new Hashtable<>();
	
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
        System.out.println("Successfully opened socket at port " + Integer.toString(DEFAULTPORT) + ".");

        // Create some "servers"
        servertable.put("someaddr1", new N64Server("Server 1", 2, "someaddr1", "tictactoe.n64"));
        servertable.put("someaddr2", new N64Server("Server 2", 4, "someaddr2", "tictactoe.n64"));
        servertable.put("someaddr3", new N64Server("Server 3", 3, "someaddr3", "tictactoe.n64"));
        
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