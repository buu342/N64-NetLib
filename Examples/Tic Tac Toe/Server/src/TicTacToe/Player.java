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
        if (sender == null)
            pkt.SetSender(0);
        else
            pkt.SetSender(sender.GetNumber());
    	this.messages.add(pkt);
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