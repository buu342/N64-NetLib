import java.net.Socket;
import java.io.DataInputStream;

public class ClientConnectionThread implements Runnable {

    TicTacToe.Game game;
    Socket clientsocket;
    TicTacToe.Player player;
    
    ClientConnectionThread(Socket socket, TicTacToe.Game game) {
        this.clientsocket = socket;
        this.game = game;
        this.player = null;
    }
    
    private boolean CheckCString(byte[] data, String str) {
        int max = Math.min(str.length(), data.length);
        for (int i=0; i<max; i++) {
            char readbyte = (char) (((int) data[i]) & 0xFF);
            if (readbyte != str.charAt(i))
                return false;
        }
        return true;
    }
    
    public void run() {
        
        // First, we have to receive a playerinfo request packet
        // This tells us that the player has the game booted on the N64,
        // and that they're ready to be assigned player data
        
        // Try to connect the player to the game
        this.player = game.ConnectPlayer();
        if (this.player == null)
            return;
        
        // Receive packets in a loop
        try {
            int attempts = 5;
            DataInputStream dis = new DataInputStream(this.clientsocket.getInputStream());
            while (true) {
                int datasize = 0;
                byte[] data = dis.readNBytes(6);
                if (!CheckCString(data, "N64PKT")) {
                    System.err.println("    Received bad packet "+data.toString());
                    attempts--;
                    if (attempts == 0) {
                        System.err.println("Too many bad packets from client "+this.clientsocket+". Disconnecting");
                        break;
                    }
                    continue;
                }
                attempts = 5;
                datasize = dis.readInt();
                data = dis.readNBytes(datasize);
                
                // Check packets here
            }
            System.out.println("Finished with "+this.clientsocket);
            this.clientsocket.close();
            dis.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
        game.DisconnectPlayer(this.player);
    }
}