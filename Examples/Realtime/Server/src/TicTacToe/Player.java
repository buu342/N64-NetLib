package TicTacToe;

import NetLib.NetLibPacket;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

public class Player {
    
    // Player info
    private int number;
    private int bitmask;
    
    // Thread communication
    private Queue<NetLibPacket> messages;

    /**
     * Object representation of a connected client, as a player
     */
    public Player() {
    	this.messages = new ConcurrentLinkedQueue<NetLibPacket>();
    }

    /**
     * Set the player's number
     * @param number  The player's number
     */
    public void SetNumber(int number) {
        this.number = number;
        this.bitmask = 1 << (number-1);
    }
    
    /**
     * Send a message to this player
     * @param sender  The player who sent the packet (or zero if the server sent it)
     * @param pkt     The packet to send to the player
     */
    public void SendMessage(Player sender, NetLibPacket pkt) {
        // We can't just send the packet directly, because otherwise we'll send the sequence data + send attempts + other internal properties over to the other client. We don't wanna do that.
        // So instead we make a copy with the same data and flags.
        NetLibPacket duplicate = new NetLibPacket(pkt.GetType(), pkt.GetData(), pkt.GetFlags());
        if (sender == null)
            duplicate.SetSender(0);
        else
            duplicate.SetSender(sender.GetNumber());
    	this.messages.add(duplicate);
    }
    
    /**
     * Get the number of this player
     * @return  The player's number
     */
    public int GetNumber() {
        return this.number;
    }
    
    /**
     * Get the bitmask for this player's number
     * @return  The bitmask for the player's number
     */
    public int GetBitMask() {
        return this.bitmask;
    }
    
    /**
     * Get the list of messages for this player
     * @return  The list of messages for this player
     */
    public Queue<NetLibPacket> GetMessages() {
        return this.messages;
    }
}