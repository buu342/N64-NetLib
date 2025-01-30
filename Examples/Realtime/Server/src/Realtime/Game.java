package Realtime;

import NetLib.NetLibPacket;

import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

public class Game implements Runnable  {
    
    // Game state
    private Player players[];
    private LinkedList<MovingObject> objs;
    
    // Thread communication
    private Queue<NetLibPacket> messages;

    /**
     * Object representation of the Ultimate TicTacToe game
     */
    public Game() {
        PreviewFrame frame = new PreviewFrame(this);
    	this.messages = new ConcurrentLinkedQueue<NetLibPacket>();
        this.players = new Player[32];
        this.objs = new LinkedList<MovingObject>();
        this.objs.add(new MovingObject(new Vector2D(320/2, 240/2)));
        System.out.println("Realtime initialized");
    }

    /**
     * Run this thread
     */
    public void run() {
        try {
            Thread.currentThread().setName("Game");
            
            while (true) {

            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Connect a player to the game
     * @return  The player struct for this client, or null if the server is full
     */
    public synchronized Player ConnectPlayer() {
        boolean foundslot = false;
        Player ply = new Player();
        for (int i=0; i<this.players.length; i++) {
            if (this.players[i] == null) {
                this.players[i] = ply;
                ply.SetNumber(i+1);
                foundslot = true;
                break;
            }
        }
        if (foundslot)
            return ply;
        return null;
    }
    
    /**
     * Send a message to this thread
     * @param sender  The player who sent the packet
     * @param pkt     The NetLibPacket to send
     */
    public void SendMessage(Player sender, NetLibPacket pkt) {
        pkt.SetSender(sender.GetNumber());
    	this.messages.add(pkt);
    }
    
    /**
     * Get the number of players active in the server
     * @return  The number of active players in the server
     */
    public int PlayerCount() {
        int count = 0;
        for (int i=0; i<this.players.length; i++)
            if (this.players[i] != null)
                count++;
        return count;
    }
    
    /**
     * Get the array of players
     * @return  The players array
     */
    public Player[] GetPlayers() {
    	return this.players;
    }

    /**
     * Disconnect a player from the server
     * @param ply  The player who disconnected
     */
    public synchronized void DisconnectPlayer(Player ply) {
        for (int i=0; i<this.players.length; i++) {
            if (this.players[i] == ply) {
                this.players[i] = null;
                return;
            }
        }
    }
    
    public LinkedList<MovingObject> GetObjects() {
        return this.objs;
    }
}