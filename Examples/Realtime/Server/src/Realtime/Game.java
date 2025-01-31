package Realtime;

import NetLib.NetLibPacket;
import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

public class Game implements Runnable  {
    
    private static int   TICKRATE = 15;
    private static float DELTATIME = 1.0f/((float)TICKRATE);
    private static long  MAXDELTA = (long)(0.25f*1E9);
    
    // Frame
    private PreviewWindow window;
    
    // Game state
    private Player players[];
    private LinkedList<MovingObject> objs;
    
    // Thread communication
    private Queue<NetLibPacket> messages;

    /**
     * Object representation of the Ultimate TicTacToe game
     */
    public Game() {
    	this.messages = new ConcurrentLinkedQueue<NetLibPacket>();
        this.players = new Player[32];
        this.objs = new LinkedList<MovingObject>();
        this.objs.add(new MovingObject(new Vector2D(320/2, 240/2)));
        this.window = new PreviewWindow(this);
        System.out.println("Realtime initialized");
    }

    /**
     * Run this thread
     */
    public void run() {
        try {
            float accumulator = 0;
            long oldtime = System.nanoTime();
            
            Thread.currentThread().setName("Game");
            while (true) {
                long curtime = System.nanoTime();
                long frametime = curtime - oldtime;
                
                // In order to prevent problems if the game slows down significantly, we will clamp the maximum timestep the simulation can take
                if (frametime > MAXDELTA)
                    frametime = MAXDELTA;
                oldtime = curtime;
                
                // Perform the update in discrete steps (ticks)
                accumulator += ((float)frametime)/1E9;
                while (accumulator >= DELTATIME) {
                    do_update();
                    accumulator -= DELTATIME;
                }
                
                // Draw the frame
                this.window.repaint();
                
                // TODO: Sleep if faster than a tick
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    private void do_update() {
        for (MovingObject obj : this.objs) {
            if (obj != null) {
                Vector2D target_offset = new Vector2D(obj.GetDirection().x*obj.GetSpeed(), obj.GetDirection().y*obj.GetSpeed());
                if (obj.GetPos().x + obj.GetSize().x/2 + target_offset.x > 320) {
                    target_offset.x -= 2*((obj.GetPos().x + obj.GetSize().x/2 + target_offset.x) - 320);
                    obj.SetDirection(new Vector2D(-obj.GetDirection().x, obj.GetDirection().y));
                }
                if (obj.GetPos().x - obj.GetSize().x/2 + target_offset.x < 0) {
                    target_offset.x -= 2*((obj.GetPos().x - obj.GetSize().x/2 + target_offset.x) - 0);
                    obj.SetDirection(new Vector2D(-obj.GetDirection().x, obj.GetDirection().y));
                }
                if (obj.GetPos().y + obj.GetSize().y/2 + target_offset.y > 240) {
                    target_offset.y -= 2*((obj.GetPos().y + obj.GetSize().y/2 + target_offset.y) - 240);
                    obj.SetDirection(new Vector2D(obj.GetDirection().x, -obj.GetDirection().y));
                }
                if (obj.GetPos().y - obj.GetSize().y/2 + target_offset.y < 0) {
                    target_offset.y -= 2*((obj.GetPos().y - obj.GetSize().y/2 + target_offset.y) - 0);
                    obj.SetDirection(new Vector2D(obj.GetDirection().x, -obj.GetDirection().y));
                }
                obj.SetPos(Vector2D.Add(obj.GetPos(), target_offset));
            }
        }
    }

    /**
     * Connect a player to the game
     * @return  The player object for this client, or null if the server is full
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