import java.net.DatagramSocket;

import NetLib.BadPacketVersionException;
import NetLib.ClientTimeoutException;
import NetLib.NetLibPacket;
import NetLib.PacketFlag;
import NetLib.S64Packet;
import NetLib.UDPHandler;
import Realtime.ClientDisconnectException;
import Realtime.GameObject;
import Realtime.PacketIDs;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ConcurrentLinkedQueue;

public class ClientConnectionThread extends Thread {

    // Constants
    private static final int HEARTBEAT_INTERVAL = 500;

    // Client state constants (too lazy to make an enum)
    private static final int CLIENTSTATE_UNCONNECTED = 0;
    private static final int CLIENTSTATE_CONNECTING = 1;
    private static final int CLIENTSTATE_CONNECTED = 2;

    // Networking
    String address;
    int port;
    DatagramSocket socket;
    UDPHandler handler;
    
    // Game
    int clientstate;
    Realtime.Game game;
    Realtime.Player player;
    
    // Thread communication
    volatile long lastmessage;
    ConcurrentLinkedQueue<byte[]> msgqueue = new ConcurrentLinkedQueue<byte[]>();
    
    /**
     * Thread for handling a client's UDP communication
     * @param socket   Socket to use for communication
     * @param address  Client address
     * @param port     Client port
     * @param game     The TicTacToe game
     */
    ClientConnectionThread(DatagramSocket socket, String address, int port, Realtime.Game game) {
        this.socket = socket;
        this.address = address;
        this.port = port;
        this.game = game;
        this.handler = null;
        this.player = null;
        this.clientstate = CLIENTSTATE_UNCONNECTED;
        this.lastmessage = 0;
    }
    
    /**
     * Send a message to this thread
     * The message should be the raw bytes received from the client
     * @param data  The data received from the client
     * @param size  The size of the received data
     */
    public void SendMessage(byte data[], int size) {
        byte[] copy = new byte[size];
        System.arraycopy(data, 0, copy, 0, size);
        this.msgqueue.add(copy);
    }

    /**
     * Update the time which we last received a packet from this client
     */
    public void UpdateClientMessageTime() {
        this.lastmessage = System.currentTimeMillis();
    }

    /**
     * Run this thread
     */
    public void run() {
        Thread.currentThread().setName("Client " + this.address + ":" + this.port);
        this.handler = new UDPHandler(this.socket, this.address, this.port);
        
        // Handle packets in a loop
        while (true) {
            try {
                byte[] data = this.msgqueue.poll();
                
                // If we received a packet, handle it based on its type
                if (data != null) {
                    if (S64Packet.IsS64PacketHeader(data)) {
                        this.HandleS64Packets(this.handler.ReadS64Packet(data));
                        return; // We can end this connection since S64 packets are one-and-done from clients
                    } else if (NetLibPacket.IsNetLibPacketHeader(data)) {
                        this.HandleNetLibPackets(this.handler.ReadNetLibPacket(data));
                    } else {
                        System.err.println("Received unknown data from client " + this.address + ":" + this.port);
                    }
                } else { // Otherwise, use this opportunity to resend missing packets/heartbeat
                    this.handler.ResendMissingPackets();
                    if (this.clientstate == CLIENTSTATE_CONNECTED && System.currentTimeMillis() - this.lastmessage > HEARTBEAT_INTERVAL)
                        this.SendHeartbeatPacket();
                    Thread.sleep(10);
                }
                
                // If we have a valid player object, send packets that were sent directly to it
                if (this.player != null) {
                    NetLibPacket nlpkt = this.player.GetMessages().poll();
                    while (nlpkt != null) {
                        this.handler.SendPacket(nlpkt);
                        nlpkt = this.player.GetMessages().poll();
                    }
                }
            } catch (ClientDisconnectException | ClientTimeoutException e) {
                if (this.clientstate == CLIENTSTATE_CONNECTED)
                {
                    // Notify other players of the disconnect if the player was valid
                    for (Realtime.Player ply : this.game.GetPlayers()) {
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
                }
                return;
            } catch (BadPacketVersionException e) {
                System.err.println(e);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * Handle S64 packets from the client
     * @param pkt  The S64 packet to handle
     * @throws ClientTimeoutException  Shouldn't happen as we are sending unreliable flags
     * @throws IOException             If an I/O error occurs
     */
    private void HandleS64Packets(S64Packet pkt) throws IOException, ClientTimeoutException {
        if (pkt == null)
            return;
        if (pkt.GetType().equals("DISCOVER")) {
            String identifier = new String(pkt.GetData(), StandardCharsets.UTF_8);
            this.handler.SendPacket(new S64Packet("DISCOVER", RealtimeServer.ToByteArray_Client(identifier), PacketFlag.FLAG_UNRELIABLE.GetInt()));
            System.out.println("Client " + this.address + ":" + this.port + " discovered server");
        }
    }

    /**
     * Handle NetLib packets from the client
     * @param pkt  The NetLib packet to handle
     * @throws ClientTimeoutException     If the packet is sent MAX_RESEND times without an acknowledgement
     * @throws ClientDisconnectException  If the client disconnected while the server was running
     * @throws IOException                If an I/O error occurs
     * @throws InterruptedException       If this function is interrupted during sleep
     */
    private void HandleNetLibPackets(NetLibPacket pkt) throws IOException, ClientDisconnectException, InterruptedException, ClientTimeoutException {
        if (pkt == null)
            return;
        
        // Heartbeats can just be ignored
        if (pkt.GetType() == PacketIDs.PACKETID_ACKBEAT.GetInt())
            return;
        
        // Handle the rest of the packets
        switch (this.clientstate) {
            // First, we have to receive a client connection request packet
            // This tells us that the player has the game booted on the N64,
            // and that they're ready to begin the clock synchronization
            case CLIENTSTATE_UNCONNECTED:
                if (pkt.GetType() != PacketIDs.PACKETID_CLIENTCONNECT.GetInt()) {
                    System.err.println("Expected client connect packet, got " + pkt.GetType() + ". Disconnecting");
                    throw new ClientDisconnectException(this.handler.GetAddress() + this.handler.GetPort());
                }
                
                // Try to connect the player to the game
                if (!this.game.CanConnectPlayer()) {
                    System.err.println("Server full");
                    this.SendServerFullPacket();
                    return;
                }
                
                // Allow this player to connect
                this.NotifyValidConnection();
                this.clientstate = CLIENTSTATE_CONNECTING;
                break;
            case CLIENTSTATE_CONNECTING:
                if (pkt.GetType() == PacketIDs.PACKETID_CLOCKSYNC.GetInt()) {
                    long time = this.game.GetGameTime();
                    ByteArrayOutputStream bytes = new ByteArrayOutputStream();
                    bytes.write(ByteBuffer.allocate(8).putLong(time).array());
                    this.handler.SendPacket(new NetLibPacket(PacketIDs.PACKETID_CLOCKSYNC.GetInt(), bytes.toByteArray()));
                    System.out.println("Sent clock packet with "+time);
                } else if (pkt.GetType() == PacketIDs.PACKETID_DONESYNC.GetInt()) {
                    
                    // Respond with the player info
                    this.player = this.game.ConnectPlayer();
                    this.SendClientInfoPacket(this.player);
                    
                    // Send the rest of the connected player's information (and notify other players of us)
                    for (Realtime.Player ply : this.game.GetPlayers()) {
                        if (ply != null && ply.GetNumber() != this.player.GetNumber()) {
                            this.SendPlayerInfoPacket(this.player, ply);
                            this.SendPlayerInfoPacket(ply, this.player);
                        }
                    }
                    
                    // Send the list of existing game objects
                    this.SendAllObjectData(this.player);
                    
                    // Done with the initial handshake, now we can go into the gameplay packet handling loop
                    System.out.println("Player " + this.player.GetNumber() + " has joined the game");
                    this.clientstate = CLIENTSTATE_CONNECTED;
                    Thread.currentThread().setName("Client " + this.player.GetNumber());
                } else {
                    System.err.println("Expected handshake packets, got " + pkt.GetType() + ". Disconnecting");
                    throw new ClientDisconnectException(this.handler.GetAddress() + this.handler.GetPort());
                }
                break;
            case CLIENTSTATE_CONNECTED:
                // Now that the player is connected, all we gotta do is act as a packet relayer to the game thread and other client threads
                
                // Relay packets to other clients or the server
                if (pkt.GetRecipients() != 0) {
                    for (Realtime.Player ply : this.game.GetPlayers())
                        if (ply != this.player && (pkt.GetRecipients() & ply.GetBitMask()) != 0)
                            ply.SendMessage(this.player, pkt);
                } else {
                    if (pkt.GetType() != PacketIDs.PACKETID_ACKBEAT.GetInt()) // Special case for heartbeats, no need to notify to the game 
                        this.game.SendMessage(this.player, pkt);
                }
                break;
        }
    }

    /**
     * Notify the connecting client that they can connect
     * @throws ClientTimeoutException     If the packet is sent MAX_RESEND times without an acknowledgement
     * @throws IOException                If an I/O error occurs
     */
    private void NotifyValidConnection() throws IOException, ClientTimeoutException {
        // TODO: Send the server tickrate
        this.handler.SendPacket(new NetLibPacket(PacketIDs.PACKETID_CLIENTCONNECT.GetInt(), null, PacketFlag.FLAG_EXPLICITACK.GetInt()));
    }

    /**
     * Notify the connecting client that the server is full
     * @throws ClientTimeoutException     If the packet is sent MAX_RESEND times without an acknowledgement
     * @throws IOException                If an I/O error occurs
     */
    private void SendServerFullPacket() throws IOException, ClientTimeoutException {
        this.handler.SendPacket(new NetLibPacket(PacketIDs.PACKETID_SERVERFULL.GetInt(), null, PacketFlag.FLAG_EXPLICITACK.GetInt()));
    }

    /**
     * Send a heartbeat packet to the client
     * @throws ClientTimeoutException     If the packet is sent MAX_RESEND times without an acknowledgement
     * @throws IOException                If an I/O error occurs
     */
    private void SendHeartbeatPacket() throws IOException, ClientTimeoutException {
        this.handler.SendPacket(new NetLibPacket(PacketIDs.PACKETID_ACKBEAT.GetInt(), null));
        this.lastmessage = System.currentTimeMillis();
    }

    /**
     * Send a player information packet to a given target
     * @param target  The destination client for the packet
     * @throws ClientTimeoutException     If the packet is sent MAX_RESEND times without an acknowledgement
     * @throws IOException                If an I/O error occurs
     */
    private void SendClientInfoPacket(Realtime.Player target) throws IOException, ClientTimeoutException {
    	NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_CLIENTINFO.GetInt(), target.GetData());
        pkt.AddRecipient(target.GetNumber());
        this.handler.SendPacket(pkt);
    }

    /**
     * Send a player information packet to a given target
     * @param target  The destination client for the packet
     * @param target  The player who's information we want to give out
     * @throws ClientTimeoutException     If the packet is sent MAX_RESEND times without an acknowledgement
     * @throws IOException                If an I/O error occurs
     */
    private void SendPlayerInfoPacket(Realtime.Player target, Realtime.Player who) throws IOException, ClientTimeoutException {
        NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERINFO.GetInt(), who.GetData());
        pkt.AddRecipient(target.GetNumber());
        if (target == who)
            this.handler.SendPacket(pkt);
        else
            target.SendMessage(who, pkt);
    }

    /**
     * Send a player information about all existing objects
     * @param target  The destination client for the packet
     * @throws ClientTimeoutException     If the packet is sent MAX_RESEND times without an acknowledgement
     * @throws IOException                If an I/O error occurs
     */
    private void SendAllObjectData(Realtime.Player target) throws IOException, ClientTimeoutException {
        for (GameObject obj : this.game.GetObjects()) {
            NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_OBJECTCREATE.GetInt(), obj.GetData());
            pkt.AddRecipient(target.GetNumber());
            this.handler.SendPacket(pkt);
        }
    }

    /**
     * Send a player disconnect packet to a given target
     * @param target  The destination client for the packet
     * @param target  The player who disconnected
     * @throws ClientTimeoutException     If the packet is sent MAX_RESEND times without an acknowledgement
     * @throws IOException                If an I/O error occurs
     */
    private void SendPlayerDisconnectPacket(Realtime.Player target, Realtime.Player who) throws IOException, ClientTimeoutException {
    	NetLibPacket pkt = new NetLibPacket(PacketIDs.PACKETID_PLAYERDISCONNECT.GetInt(), new byte[]{(byte)who.GetNumber()});
        pkt.AddRecipient(target.GetNumber());
        if (target == who)
            this.handler.SendPacket(pkt);
        else
            target.SendMessage(who, pkt);
    }
}