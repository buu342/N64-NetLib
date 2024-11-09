import java.net.Socket;

import NetLib.NetLibPacket;
import TicTacToe.PacketIDs;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class ClientConnectionThread implements Runnable {

    TicTacToe.Game game;
    Socket clientsocket;
    TicTacToe.Player player;
    
    ClientConnectionThread(Socket socket, TicTacToe.Game game) {
        this.clientsocket = socket;
        this.game = game;
        this.player = null;
    }
    
    private void SendServerFullPacket(DataOutputStream dos) throws IOException {
    	NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_SERVERFULL.GetInt(), null);
        pkt.WritePacket(dos);
    }
    
    private void SendPlayerInfoPacket(DataOutputStream dos, TicTacToe.Player target, TicTacToe.Player ply) throws IOException {
    	NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERINFO.GetInt(), new byte[]{(byte)ply.GetNumber()});
        pkt.AddRecipient(target.GetNumber());
        pkt.WritePacket(dos);
    }
    
    private void SendPlayerDisconnectPacket(DataOutputStream dos, TicTacToe.Player target, TicTacToe.Player ply) throws IOException {
    	NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERDISCONNECT.GetInt(), new byte[]{(byte)ply.GetNumber()});
        pkt.AddRecipient(target.GetNumber());
        pkt.WritePacket(dos);
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
                        System.err.println("Too many bad packets from client " + this.clientsocket + ". Disconnecting");
                        return;
                    }
                    continue;
                }
                
                // Now read the client request packet
                if (pkt.GetID() != PacketIDs.PACKETID_CLIENTCONNECT.GetInt())
                {
                    System.err.println("Expected client connect packet, got " + pkt.GetID() + ". Disconnecting");
                    return;
                }
                
                // Try to connect the player to the game
                this.player = this.game.ConnectPlayer();
                if (this.player == null) {
                	this.SendServerFullPacket(dos);
                    return;
                }
                
                // Respond with the player's own info
                this.SendPlayerInfoPacket(dos, this.player, this.player);
                
                // Also send the rest of the connected player's information
                for (TicTacToe.Player ply : this.game.GetPlayers())
                	if (ply.GetNumber() != this.player.GetNumber())
                		this.SendPlayerInfoPacket(dos, this.player, ply);
                
                // Done with the initial handshake, now we can go into the gameplay packet loop
                break;
            }
        } catch (Exception e) {
            e.printStackTrace();
            return;
        }
        
        // Send/Receive packets in a loop
        try {
            int attempts = 5;
            while (true)
            {
            	NetLibPacket pkt;
            	boolean donesomething = false;
            	
            	// Check for incoming data from the N64
            	while (dis.available() > 0)
            	{
	                pkt = NetLibPacket.ReadPacket(dis);
	                if (pkt == null) {
	                    System.err.println("    Received bad packet");
	                    attempts--;
	                    if (attempts == 0) {
	                        System.err.println("Too many bad packets from client " + this.clientsocket + ". Disconnecting");
	                        break;
	                    }
	                    continue;
	                }
	                attempts = 5;
	                
	                // Relay packets to other clients or the server
	                if (pkt.GetRecipients() != 0)
	                {
	                	for (TicTacToe.Player ply : this.game.GetPlayers())
	                		if ((pkt.GetRecipients() & ply.GetBitMask()) != 0)
	                			ply.SendMessage(pkt);
	                }
	                else
	                	this.game.SendMessage(pkt);
	                donesomething = true;
            	}
                
                // Send outgoing data to the N64
            	pkt = this.player.GetMessages().poll();
            	while (pkt != null)
                {
            		pkt.WritePacket(dos);
                	pkt = this.player.GetMessages().poll();
                	donesomething = true;
                }
            	
            	// Sleep a bit if no packets were sent or received
            	if (!donesomething) {
            		Thread.sleep(50);
            	}
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        // Notify other players of the disconnect
        for (TicTacToe.Player ply : this.game.GetPlayers())
        {
        	if (ply.GetNumber() != this.player.GetNumber())
        	{
        		try {
        			this.SendPlayerDisconnectPacket(dos, this.player, ply);
        		} catch (Exception e) {
                    e.printStackTrace();
        		}
        	}
        }
        game.DisconnectPlayer(this.player);
    }
}