package Realtime;

import NetLib.NetLibPacket;
import java.awt.Toolkit;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.LinkedList;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicInteger;

public class Game implements Runnable  {

    public static final int    MAXPLAYERS = 32;
    private static final int   TICKRATE = 15;
    private static final float DELTATIME = 1.0f/((float)TICKRATE);
    private static final long  MAXDELTA = (long)(0.25f*1E9);
    
    // Frame
    private PreviewWindow window;
    
    // Game state
    private Player players[];
    private LinkedList<GameObject> objs;
    private GameObject obj_npc;
    private long gametime;
    private static AtomicInteger counter = new AtomicInteger();
    
    // Thread communication
    private Queue<NetLibPacket> messages;

    /**
     * Object representation of the Ultimate TicTacToe game
     */
    public Game() {
    	this.messages = new ConcurrentLinkedQueue<NetLibPacket>();
        this.players = new Player[32];
        this.objs = new LinkedList<GameObject>();
        this.obj_npc = new GameObject(new Vector2D(320/2, 240/2));
        this.obj_npc.SetSpeed(5);
        this.objs.add(this.obj_npc);
        this.window = new PreviewWindow(this);
        this.gametime = 0;
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
                this.gametime += frametime;
                accumulator += ((float)frametime)/1E9;
                while (accumulator >= DELTATIME) {
                    do_update();
                    send_updates();
                    accumulator -= DELTATIME;
                }
                
                // Draw the frame
                this.window.repaint();
                Toolkit.getDefaultToolkit().sync();
                
                // Sleep for a bit
                Thread.sleep(10);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    private void do_update() {
        
        // Check for player messages
        NetLibPacket pkt = this.messages.poll();
        while (pkt != null) {
            Player sender = this.players[pkt.GetSender()-1];
            if (sender != null) {
                try {
                    if (pkt.GetType() == PacketIDs.PACKETID_CLIENTINPUT.GetInt()) {
                        final float MAXSTICK = 80, MAXSPEED = 5;
                        float stickx, sticky;
                        Vector2D dir;
                        GameObject obj = sender.GetObject();
                        stickx = (float)pkt.GetData()[8];
                        sticky = -(float)pkt.GetData()[9];
                        dir = new Vector2D(stickx, sticky);
                        dir.Normalize();
                        obj.SetDirection(dir);
                        obj.SetSpeed((int)(((float)Math.sqrt(stickx*stickx + sticky*sticky)/MAXSTICK)*MAXSPEED));
                    }
                } catch (Exception e) {
                    
                }
            }
            pkt = this.messages.poll();
        }
        
        // Update object positions
        for (GameObject obj : this.objs) {
            if (obj != null) {
                Vector2D target_offset = new Vector2D(obj.GetDirection().GetX()*obj.GetSpeed(), obj.GetDirection().GetY()*obj.GetSpeed());
                if (obj.GetPos().GetX() + obj.GetSize().GetX()/2 + target_offset.GetX() > 320) {
                    target_offset.SetX(target_offset.GetX() - 2*((obj.GetPos().GetX() + obj.GetSize().GetX()/2 + target_offset.GetX()) - 320));
                    obj.SetDirection(new Vector2D(-obj.GetDirection().GetX(), obj.GetDirection().GetY()));
                }
                if (obj.GetPos().GetX() - obj.GetSize().GetX()/2 + target_offset.GetX() < 0) {
                    target_offset.SetX(target_offset.GetX() - 2*((obj.GetPos().GetX() - obj.GetSize().GetX()/2 + target_offset.GetX()) - 0));
                    obj.SetDirection(new Vector2D(-obj.GetDirection().GetX(), obj.GetDirection().GetY()));
                }
                if (obj.GetPos().GetY() + obj.GetSize().GetY()/2 + target_offset.GetY() > 240) {
                    target_offset.SetY(target_offset.GetY() - 2*((obj.GetPos().GetY() + obj.GetSize().GetY()/2 + target_offset.GetY()) - 240));
                    obj.SetDirection(new Vector2D(obj.GetDirection().GetX(), -obj.GetDirection().GetY()));
                }
                if (obj.GetPos().GetY() - obj.GetSize().GetY()/2 + target_offset.GetY() < 0) {
                    target_offset.SetY(target_offset.GetY() - 2*((obj.GetPos().GetY() - obj.GetSize().GetY()/2 + target_offset.GetY()) - 0));
                    obj.SetDirection(new Vector2D(obj.GetDirection().GetX(), -obj.GetDirection().GetY()));
                }
                obj.SetPos(Vector2D.Add(obj.GetPos(), target_offset));
            }
        }
    }
    
    private void send_updates() {
        for (GameObject obj : this.objs) {
            try {
                ByteArrayOutputStream bytes = new ByteArrayOutputStream();
				bytes.write(ByteBuffer.allocate(4).putInt(obj.GetID()).array());
	        	if (obj.GetPos().GetX() != obj.GetPos().GetPreviousX() || obj.GetPos().GetY() != obj.GetPos().GetPreviousY()) {
					bytes.write(ByteBuffer.allocate(1).put((byte)0).array());
					bytes.write(ByteBuffer.allocate(4).putFloat(obj.GetPos().GetX()).array());
					bytes.write(ByteBuffer.allocate(4).putFloat(obj.GetPos().GetY()).array());
	        	}
	        	if (obj.GetDirection().GetX() != obj.GetDirection().GetPreviousX() || obj.GetDirection().GetY() != obj.GetDirection().GetPreviousY()) {
					bytes.write(ByteBuffer.allocate(1).put((byte)1).array());
					bytes.write(ByteBuffer.allocate(4).putFloat(obj.GetDirection().GetX()).array());
					bytes.write(ByteBuffer.allocate(4).putFloat(obj.GetDirection().GetY()).array());
	        	}
	        	if (obj.GetSize().GetX() != obj.GetSize().GetPreviousX() || obj.GetSize().GetY() != obj.GetSize().GetPreviousY()) {
					bytes.write(ByteBuffer.allocate(1).put((byte)2).array());
					bytes.write(ByteBuffer.allocate(4).putFloat(obj.GetSize().GetX()).array());
					bytes.write(ByteBuffer.allocate(4).putFloat(obj.GetSize().GetY()).array());
	        	}
	        	if (obj.GetOldSpeed() != obj.GetSpeed()) {
					bytes.write(ByteBuffer.allocate(1).put((byte)3).array());
					bytes.write(ByteBuffer.allocate(4).putFloat(obj.GetSpeed()).array());
	        	}

	        	// Network if we had a change in object properties
	        	if (bytes.size() > 4) {
	        		for (Player ply : this.players) {
	        			if (ply != null) {
	        				ply.SendMessage(null, new NetLibPacket(PacketIDs.PACKETID_OBJECTUPDATE.GetInt(), bytes.toByteArray()));
	        			}
	        		}
	        	}
			} catch (IOException e) {
				e.printStackTrace();
			}
        }
    }
    
    /**
     * Get the game time
     * @return  The game time
     */
    public long GetGameTime() {
        return this.gametime;
    }

    /**
     * Gets a list of all existing game objects
     * @return  The list of game objects
     */
    public LinkedList<GameObject> GetObjects() {
        return this.objs;
    }

    /**
     * Gets a unique object ID.
     * @return  The unique object ID.
     */
    public static int NewObjectID() {
        return counter.getAndIncrement();
    }

    /**
     * Checks whether the player can join the game
     * @return  False if the server is full, true if otherwise
     */
    public synchronized boolean CanConnectPlayer() {
        for (int i=0; i<this.players.length; i++)
            if (this.players[i] == null)
                return true;
        return false;
    }

    /**
     * Connect a player to the game
     * @return  The player object for this client, or null if the server is full
     */
    public synchronized Player ConnectPlayer() {
        for (int i=0; i<this.players.length; i++) {
            if (this.players[i] == null) {
                Player ply = new Player();
                this.players[i] = ply;
                ply.SetNumber(i+1);
                this.objs.add(ply.GetObject());
                return ply;
            }
        }
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
        if (ply != null) {
            this.objs.remove(ply.GetObject());
            this.players[ply.GetNumber()-1] = null;
        }
    }
}