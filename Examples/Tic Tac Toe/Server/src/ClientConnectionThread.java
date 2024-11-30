import java.net.DatagramSocket;
import NetLib.ClientTimeoutException;
import NetLib.NetLibPacket;
import NetLib.PacketFlag;
import NetLib.S64Packet;
import NetLib.UDPHandler;
import TicTacToe.ClientDisconnectException;
import TicTacToe.PacketIDs;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ConcurrentLinkedQueue;

public class ClientConnectionThread extends Thread {

    private static final int HEARTBEAT_INTERVAL = 5000;
    
    private static final int CLIENTSTATE_UNCONNECTED = 0;
    private static final int CLIENTSTATE_CONNECTED = 1;

    String address;
    int port;
    DatagramSocket socket;
    UDPHandler handler;
    int clientstate;
    TicTacToe.Game game;
    TicTacToe.Player player;
    volatile long lastmessage;
    ConcurrentLinkedQueue<byte[]> msgqueue = new ConcurrentLinkedQueue<byte[]>();
    
    ClientConnectionThread(DatagramSocket socket, String address, int port, TicTacToe.Game game) {
        this.socket = socket;
        this.address = address;
        this.port = port;
        this.handler = null;
        this.player = null;
        this.clientstate = CLIENTSTATE_UNCONNECTED;
        this.lastmessage = 0;
    }
    
    public void SendMessage(byte data[], int size) {
        byte[] copy = new byte[size];
        System.arraycopy(data, 0, copy, 0, size);
        this.msgqueue.add(copy);
        
    }
    
    public void UpdateClientMessageTime() {
        this.lastmessage = System.currentTimeMillis();
    }

    public void run() {
        Thread.currentThread().setName("Client " + this.address + ":" + this.port);
        this.handler = new UDPHandler(this.socket, this.address, this.port);
        
        while (true) {
            try {
                byte[] data = this.msgqueue.poll();
                if (data != null) {
                    if (S64Packet.IsS64PacketHeader(data)) {
                        this.HandleS64Packets(this.handler.ReadS64Packet(data));
                        return; // We can end this connection since S64 packets are one-and-done from clients
                    } else if (NetLibPacket.IsNetLibPacketHeader(data)) {
                        this.HandleNetLibPackets(this.handler.ReadNetLibPacket(data));
                    } else {
                        System.err.println("Received unknown data from client " + this.address + ":" + this.port);
                    }
                } else {
                    this.handler.ResendMissingPackets();
                    Thread.sleep(10);
                }
            } catch (ClientTimeoutException e) {
                e.printStackTrace();
                return;
            } catch (ClientDisconnectException e) {
                
                // Notify other players of the disconnect
                for (TicTacToe.Player ply : this.game.GetPlayers()) {
                    if (ply != null && ply.GetNumber() != this.player.GetNumber()) {
                        try {
                            this.SendPlayerDisconnectPacket(ply, this.player);
                        } catch (Exception e2) {
                            e2.printStackTrace();
                        }
                    }
                }

                // Kill this thread
                System.out.println("Player " + this.player.GetNumber() + " disconnected");
                game.DisconnectPlayer(this.player);
                return;
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }
    
    private void HandleS64Packets(S64Packet pkt) throws IOException, ClientTimeoutException {
        if (pkt.GetType().equals("DISCOVER")) {
            String identifier = new String(pkt.GetData(), StandardCharsets.UTF_8);
            this.handler.SendPacket(new S64Packet("DISCOVER", TicTacToeServer.ToByteArray_Client(identifier), PacketFlag.FLAG_UNRELIABLE.GetInt()));
            System.out.println("Client " + this.address + ":" + this.port + " discovered server");
        }
    }
    
    private void HandleNetLibPackets(NetLibPacket pkt) throws IOException, ClientDisconnectException, InterruptedException, ClientTimeoutException {
        switch (this.clientstate) {
            // First, we have to receive a client connection request packet
            // This tells us that the player has the game booted on the N64,
            // and that they're ready to be assigned player data
            // as well as to receive information about other connected players
            case CLIENTSTATE_UNCONNECTED: {
                if (pkt.GetType() != PacketIDs.PACKETID_CLIENTCONNECT.GetInt()) {
                    System.err.println("Expected client connect packet, got " + pkt.GetType() + ". Disconnecting");
                    throw new ClientDisconnectException(this.handler.GetAddress() + this.handler.GetPort());
                }
                
                // Try to connect the player to the game
                this.player = this.game.ConnectPlayer();
                if (this.player == null) {
                    System.err.println("Server full");
                    this.SendServerFullPacket();
                    return;
                }
                
                // Respond with the player's own info
                this.SendPlayerInfoPacket(this.player, this.player);
                
                // Also send the rest of the connected player's information (and notify other players of us)
                for (TicTacToe.Player ply : this.game.GetPlayers()) {
                    if (ply != null && ply.GetNumber() != this.player.GetNumber()) {
                        this.SendPlayerInfoPacket(this.player, ply);
                        this.SendPlayerInfoPacket(ply, this.player);
                    }
                }
                
                // Done with the initial handshake, now we can go into the gameplay packet loop
                System.out.println("Player " + this.player.GetNumber() + " has joined the game");
                
                this.clientstate = CLIENTSTATE_CONNECTED;
                Thread.currentThread().setName("Client " + this.player.GetNumber());
                break;
            }
            case CLIENTSTATE_CONNECTED: {
                // Relay packets to other clients or the server
                if (pkt.GetRecipients() != 0) {
                    for (TicTacToe.Player ply : this.game.GetPlayers())
                        if (ply != this.player && (pkt.GetRecipients() & ply.GetBitMask()) != 0)
                            ply.SendMessage(this.player, pkt);
                } else {
                    if (pkt.GetType() != PacketIDs.PACKETID_ACKBEAT.GetInt()) // Special case for heartbeats, no need to notify to the game 
                        this.game.SendMessage(this.player, pkt);
                }
                
                // Send outgoing data to the N64
                pkt = this.player.GetMessages().poll();
                while (pkt != null) {
                    this.handler.SendPacket(pkt);
                    pkt = this.player.GetMessages().poll();
                }
                
                // Handle heartbeats
                if (System.currentTimeMillis() - this.lastmessage > HEARTBEAT_INTERVAL)
                    this.SendHeartbeatPacket();
                break;
            }
        }
    }

    private void SendServerFullPacket() throws IOException, ClientTimeoutException {
        this.handler.SendPacket(new NetLibPacket(PacketIDs.PACKETID_SERVERFULL.GetInt(), null, PacketFlag.FLAG_EXPLICITACK.GetInt()));
    }

    private void SendHeartbeatPacket() throws IOException, ClientTimeoutException {
        this.handler.SendPacket(new NetLibPacket(PacketIDs.PACKETID_ACKBEAT.GetInt(), null));
        this.lastmessage = System.currentTimeMillis();
    }
    
    private void SendPlayerInfoPacket(TicTacToe.Player target, TicTacToe.Player who) throws IOException, InterruptedException, ClientTimeoutException {
    	NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERINFO.GetInt(), new byte[]{(byte)who.GetNumber()});
        pkt.AddRecipient(target.GetNumber());
        if (target == who)
            this.handler.SendPacket(pkt);
        else
            target.SendMessage(who, pkt);
    }
    
    private void SendPlayerDisconnectPacket(TicTacToe.Player target, TicTacToe.Player who) throws IOException, ClientTimeoutException {
    	NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERDISCONNECT.GetInt(), new byte[]{(byte)who.GetNumber()});
        pkt.AddRecipient(target.GetNumber());
        if (target == who)
            this.handler.SendPacket(pkt);
        else
            target.SendMessage(who, pkt);
    }
}