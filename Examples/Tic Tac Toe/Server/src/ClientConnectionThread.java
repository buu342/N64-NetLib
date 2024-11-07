import java.net.Socket;
import NetLib.USBPacket;
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
                USBPacket pkt = USBPacket.ReadPacket(dis);
                if (pkt == null) {
                    System.err.println("    Received bad packet");
                    attempts--;
                    if (attempts == 0) {
                        System.err.println("Too many bad packets from client "+this.clientsocket+". Disconnecting");
                        break;
                    }
                    continue;
                }
                attempts = 5;
                
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