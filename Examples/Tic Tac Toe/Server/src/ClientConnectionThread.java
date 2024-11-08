import java.net.Socket;

import NetLib.NetLibPacket;
import NetLib.USBPacket;
import TicTacToe.PacketIDs;

import java.io.DataInputStream;
import java.io.DataOutputStream;

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
        DataInputStream dis;
        DataOutputStream dos;
        
        // First, we have to receive a client connection request packet
        // This tells us that the player has the game booted on the N64,
        // and that they're ready to be assigned player data
        // as well as to receive information about other connected players
        try {
            int attempts = 5;
            dis = new DataInputStream(this.clientsocket.getInputStream());
            dos = new DataOutputStream(this.clientsocket.getOutputStream());
            while (true) {
                NetLibPacket pkt = NetLibPacket.ReadPacket(dis);
                
                // Try to read a USB packet
                if (pkt == null) {
                    System.err.println("    Received bad packet");
                    attempts--;
                    if (attempts == 0) {
                        System.err.println("Too many bad packets from client "+this.clientsocket+". Disconnecting");
                        return;
                    }
                    continue;
                }
                
                // Now read the client request packet
                if (pkt.GetID() != PacketIDs.PACKETID_CLIENTCONNECT.GetInt())
                {
                    System.err.println("Expected client connect packet, got "+pkt.GetID()+". Disconnecting");
                    return;
                }
                
                // Try to connect the player to the game
                this.player = game.ConnectPlayer();
                if (this.player == null)
                    return;
                
                // Respond with the player's own info
                pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERINFO.GetInt(), new byte[]{(byte)this.player.GetNumber()});
                pkt.AddRecipient(this.player.GetNumber());
                pkt.WritePacket(dos);
                
                // Also send the rest of the connected player's information
                // TODO: This
                
                // Done with the initial handshake, now we can go into the gameplay packet loop
                break;
            }
        } catch (Exception e) {
            e.printStackTrace();
            return;
        }
        
        // Receive packets in a loop
        try {
            int attempts = 5;
            while (true) {
                NetLibPacket pkt = NetLibPacket.ReadPacket(dis);
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
            dos.close();
            dis.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
        game.DisconnectPlayer(this.player);
    }
}