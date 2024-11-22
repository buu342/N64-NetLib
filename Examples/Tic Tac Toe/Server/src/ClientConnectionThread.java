import java.net.DatagramSocket;
import java.net.Socket;
import NetLib.NetLibPacket;
import NetLib.S64Packet;
import NetLib.UDPHandler;
import TicTacToe.PacketIDs;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.SocketException;
import java.util.concurrent.ConcurrentLinkedQueue;

public class ClientConnectionThread implements Runnable {

    private static final int HEARTBEAT_INTERVAL = 5000;
    private static final int HEARTBEAT_WAIT = 5000;
    private static final int HBSTATE_WAIT = 0;
    private static final int HBSTATE_SENT = 1;

    String address;
    int port;
    DatagramSocket socket;
    UDPHandler handler;
    TicTacToe.Game game;
    TicTacToe.Player player;
    ConcurrentLinkedQueue<byte[]> msgqueue = new ConcurrentLinkedQueue<byte[]>();
    
    ClientConnectionThread(DatagramSocket socket, String address, int port, TicTacToe.Game game) {
        this.socket = socket;
        this.address = address;
        this.port = port;
        this.handler = null;
        this.player = null;
    }
    
    public void SendMessage(byte data[], int size) {
        byte[] copy = new byte[size];
        System.arraycopy(data, 0, copy, 0, size);
        this.msgqueue.add(copy);
    }

    public void run() {
        this.handler = new UDPHandler(this.socket, this.address, this.port);
        
        while (true) {
            try {
                byte[] data = this.msgqueue.poll();
                if (data != null) {
                    if (this.handler.IsS64Packet(data)) {
                        this.HandleS64Packets(this.handler.ReadS64Packet(data));
                    } else if (this.handler.IsNetLibPacket(data)) {
                        //this.HandleNetLibPackets(this.handler.ReadNetLibPacket(data));
                    } else {
                        System.err.println("Received unknown data from client " + this.address + ":" + this.port);
                    }
                } else {
                    Thread.sleep(500);
                }
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
    
    private void HandleS64Packets(S64Packet pkt) {
        if (pkt == null)
            return;
        if (pkt.GetType().equals("DISCOVER")) {
            // TODO:
        }
    }
    
    private void ReadNetLibPacket(NetLibPacket pkt) {
        // TODO:
    }
    
    /*
    private void SendServerFullPacket(DataOutputStream dos) throws IOException {
    	NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_SERVERFULL.GetInt(), null);
        pkt.WritePacket(dos);
    }
    
    private void SendHeartbeatPacket(DataOutputStream dos) throws IOException {
        NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_HEARTBEAT.GetInt(), null);
        pkt.WritePacket(dos);
    }
    
    private void SendPlayerInfoPacket(DataOutputStream dos, TicTacToe.Player target, TicTacToe.Player who) throws IOException {
    	NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERINFO.GetInt(), new byte[]{(byte)who.GetNumber()});
        pkt.AddRecipient(target.GetNumber());
        if (target == who)
            pkt.WritePacket(dos);
        else
            target.SendMessage(who, pkt);
    }
    
    private void SendPlayerDisconnectPacket(DataOutputStream dos, TicTacToe.Player target, TicTacToe.Player who) throws IOException {
    	NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERDISCONNECT.GetInt(), new byte[]{(byte)who.GetNumber()});
        pkt.AddRecipient(target.GetNumber());
        if (target == who)
            pkt.WritePacket(dos);
        else
            target.SendMessage(who, pkt);
    }
    
    private void HandleMasterPing(DataInputStream dis, DataOutputStream dos) throws IOException {
        S64Packet pkt;
        dis.readNBytes(3); // skip the rest of the header
        pkt = S64Packet.ReadPacket(dis, true);
        System.out.println("Received ping from master server");
        if (pkt.GetType().equals("PING")) {
            pkt = new S64Packet("PING", new byte[]{(byte)this.game.PlayerCount()});
            pkt.WritePacket(dos);
        }
        System.out.println("Finished ponging master server");
    }
    
    public void run() {
        DataInputStream dis;
        DataOutputStream dos;
        Thread.currentThread().setName("Connecting Client " + this.clientsocket);
        
        // First, we have to receive a client connection request packet
        // This tells us that the player has the game booted on the N64,
        // and that they're ready to be assigned player data
        // as well as to receive information about other connected players
        try {
            int attempts = 5;
            dis = new DataInputStream(this.clientsocket.getInputStream());
            dos = new DataOutputStream(this.clientsocket.getOutputStream());
            while (true) {
                byte[] data = dis.readNBytes(3);
                
                // Handle Master Server pings first
                if (!NetLibPacket.IsNetLibPacketHeader(data)) {
                    HandleMasterPing(dis, dos);
                    dis.close();
                    dos.close();
                    this.clientsocket.close();
                    return;
                }
                NetLibPacket pkt = NetLibPacket.ReadPacket(dis, true);
                
                // Try to read a USB packet
                if (pkt == null) {
                    System.err.println("    Received bad packet");
                    attempts--;
                    if (attempts == 0) {
                        System.err.println("Too many bad packets from client " + this.clientsocket + ". Disconnecting");
                        dis.close();
                        dos.close();
                        this.clientsocket.close();
                        return;
                    }
                    continue;
                }
                
                // Now read the client request packet
                if (pkt.GetID() != PacketIDs.PACKETID_CLIENTCONNECT.GetInt()) {
                    System.err.println("Expected client connect packet, got " + pkt.GetID() + ". Disconnecting");
                    dis.close();
                    dos.close();
                    this.clientsocket.close();
                    return;
                }
                
                // Try to connect the player to the game
                this.player = this.game.ConnectPlayer();
                if (this.player == null) {
                    System.err.println("Server full");
                	this.SendServerFullPacket(dos);
                    return;
                }
                Thread.currentThread().setName("Client " + this.player.GetNumber());
                
                // Respond with the player's own info
                this.SendPlayerInfoPacket(dos, this.player, this.player);
                
                // Also send the rest of the connected player's information (and notify other players of us)
                for (TicTacToe.Player ply : this.game.GetPlayers()) {
                	if (ply != null && ply.GetNumber() != this.player.GetNumber()) {
                	    this.SendPlayerInfoPacket(dos, this.player, ply);
                		this.SendPlayerInfoPacket(dos, ply, this.player);
                	}
                }
                
                // Done with the initial handshake, now we can go into the gameplay packet loop
                System.out.println("Player " + this.player.GetNumber() + " has joined the game");
                break;
            }
        } catch (Exception e) {
            e.printStackTrace();
            System.err.println("Client disconnected");
            return;
        }
        
        // Send/Receive gameplay packets in a loop
        try {
            int attempts = 5;
            int hbstate = HBSTATE_WAIT;
            long lasthbtime = System.currentTimeMillis();
            while (true) {
            	NetLibPacket pkt;
            	boolean donesomething = false;
            	
            	// Check for incoming data from the N64
            	while (dis.available() > 0) {
	                pkt = NetLibPacket.ReadPacket(dis);
	                if (pkt == null) {
	                    System.err.println("    Received bad packet from Player " + this.player.GetNumber());
	                    attempts--;
	                    if (attempts == 0) {
	                        System.err.println("Too many bad packets from Player " + this.player.GetNumber() + ". Disconnecting");
	                        break;
	                    }
	                    continue;
	                }
	                attempts = 5;
	                
	                // Relay packets to other clients or the server
	                if (pkt.GetRecipients() != 0) {
	                	for (TicTacToe.Player ply : this.game.GetPlayers())
	                		if (ply != this.player && (pkt.GetRecipients() & ply.GetBitMask()) != 0)
	                			ply.SendMessage(this.player, pkt);
	                } else {
	                    if (pkt.GetID() != PacketIDs.PACKETID_HEARTBEAT.GetInt()) // Special case for heartbeats, no need to notify to the game 
	                        this.game.SendMessage(this.player, pkt);
	                }
	                donesomething = true;
	                
	                // Reset the heartbeat timer since we know the player is alive, as it sent something from the N64
	                lasthbtime = System.currentTimeMillis();
	                hbstate = HBSTATE_WAIT;
            	}
                
                // Send outgoing data to the N64
            	pkt = this.player.GetMessages().poll();
            	while (pkt != null) {
            		pkt.WritePacket(dos);
                	pkt = this.player.GetMessages().poll();
                	donesomething = true;
                }
            	
            	// Sleep a bit to chill the CPU if no packets were sent or received from this client
            	if (!donesomething) {
            	    // Heartbeat if we must
            	    if (hbstate == HBSTATE_WAIT && System.currentTimeMillis() - lasthbtime > HEARTBEAT_INTERVAL) {
            	        this.SendHeartbeatPacket(dos);
            	        hbstate = HBSTATE_SENT;
            	        lasthbtime = System.currentTimeMillis();
            	    } else if (hbstate == HBSTATE_SENT && System.currentTimeMillis() - lasthbtime > HEARTBEAT_WAIT) {
        	            System.err.println("No heartbeat reply from Player " + this.player.GetNumber() + ".");
        	            break;
            	    }
            	    
            	    // Sleep
            		Thread.sleep(50);
            	}
            }
        } catch (SocketException e) {
            System.err.println("Player " + this.player.GetNumber() + " unresponsive");
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        // Notify other players of the disconnect
        for (TicTacToe.Player ply : this.game.GetPlayers()) {
        	if (ply != null && ply.GetNumber() != this.player.GetNumber()) {
        		try {
        			this.SendPlayerDisconnectPacket(dos, ply, this.player);
        		} catch (Exception e) {
                    e.printStackTrace();
        		}
        	}
        }

        System.out.println("Player " + this.player.GetNumber() + " disconnected");
        game.DisconnectPlayer(this.player);
    }
    */
}