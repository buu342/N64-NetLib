package TicTacToe;

import NetLib.NetLibPacket;
import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

public class Player {
    private int number;
    private int bitmask;
    private Queue<NetLibPacket> messages;
    
    public Player() {
    	this.messages = new ConcurrentLinkedQueue<NetLibPacket>();
    }
    
    public void SetNumber(int number) {
        this.number = number;
        this.bitmask = 1 << (number-1);
    }
    
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
    
    public int GetNumber() {
        return this.number;
    }
    
    public int GetBitMask() {
        return this.bitmask;
    }
    
    public Queue<NetLibPacket> GetMessages() {
        return this.messages;
    }
}