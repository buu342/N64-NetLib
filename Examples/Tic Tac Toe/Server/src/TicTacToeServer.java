import java.net.ServerSocket;
import java.net.Socket;
import java.io.IOException;

public class TicTacToeServer {
	
	private static final String MASTER_DEFAULTADDRESS = "127.0.0.1";
	private static final String MASTER_DEFAULTPORT = "6464";
	
	private static String name;
	private static String rompath;
	private static int playercount;
	
	public static void main(String args[]) throws IOException {
		ServerSocket ss = null;
		boolean isrunning = true;

        // Validate program arguments
		
		// Try to open the port
        try {
        	ss = new ServerSocket();
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
}