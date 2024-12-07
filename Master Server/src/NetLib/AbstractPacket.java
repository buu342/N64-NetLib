package NetLib;

import java.io.IOException;

public abstract class AbstractPacket {
    
    public static final int    PACKET_MAXSIZE    = 4096;
    protected static final int PACKET_MAXSEQNUM  = 0xFFFF;

    protected int version;
    protected int flags;
    protected short seqnum;
    protected short ack;
    protected short ackbitfield;
    protected short size;
    protected byte data[];
    protected long sendtime;
    protected int attempts;
    
    public AbstractPacket(int version, byte data[], int flags, short seqnum, short ack, short ackbitfield)
    {
        this.version = version;
        this.flags = flags;
        this.sendtime = 0;
        this.data = data;
        if (data != null)
            this.size = (short)data.length;
        else
            this.size = 0;
        if (this.size > PACKET_MAXSIZE)
            System.err.println("Packet size exceeds N64 library's capacity!");
        this.seqnum = seqnum;
        this.ack = ack;
        this.ackbitfield = ackbitfield;
        this.sendtime = 0;
        this.attempts = 0;
    }
    
    public AbstractPacket(int version, byte data[], int flags) {
        this(version, data, flags, (short)0, (short)0, (short)0);
    }
    
    public AbstractPacket(int version, byte data[]) {
        this(version, data, 0, (short)0, (short)0, (short)0);
    }
    
    abstract public String toString();
    
    protected static boolean SequenceGreaterThan(int s1, int s2) {
        return ((s1 > s2) && (s1 - s2 <= ((PACKET_MAXSEQNUM/2)+1))) || ((s1 < s2) && (s2 - s1 > ((PACKET_MAXSEQNUM/2)+1)));
    }
    
    protected static int SequenceIncrement(int seq) {
        return (seq + 1) % (PACKET_MAXSEQNUM + 1);
    }
    
    protected static int SequenceDelta(int s1, int s2) {
        int delta = (s1 - s2);
        if (delta < 0)
            delta += PACKET_MAXSEQNUM+1;
        return delta;
    }
    
    protected static boolean CheckCString(byte[] data, String str) {
        int max = Math.min(str.length(), data.length);
        for (int i=0; i<max; i++) {
            char readbyte = (char) (((int) data[i]) & 0xFF);
            if (readbyte != str.charAt(i))
                return false;
        }
        return true;
    }
    
    protected static short getShort(byte[] arr) {
        return (short) ((0xff & arr[0]) << 8 | (0xff & arr[1]));
    }
    
    protected static int getInt(byte[] arr) {
        return ((arr[0] & 0xFF) << 24) | 
                ((arr[1] & 0xFF) << 16) | 
                ((arr[2] & 0xFF) << 8 ) | 
                ((arr[3] & 0xFF) << 0 );
    }
    
    abstract public byte[] GetBytes();
    
    public int GetVersion() {
        return this.version;
    }
    
    public int GetSize() {
        return this.size;
    }
    
    public byte[] GetData() {
        return this.data;
    }
    
    public short GetSequenceNumber() {
        return this.seqnum;
    }
    
    public boolean IsAcked(short number) {
        if (this.ack == number)
            return true;
        return ((this.ackbitfield & (1 << (SequenceDelta(this.ack, number) - 1))) != 0);
    }
    
    public int GetFlags() {
        return this.flags;
    }
    
    public long GetSendTime() {
        return System.currentTimeMillis() - this.sendtime;
    }
    
    public int GetSendAttempts() {
        return this.attempts;
    }
    
    abstract public boolean IsAckBeat();
    
    public void EnableFlags(int flags) {
        this.flags |= flags;
    }
    
    public void SetSequenceNumber(short seqnum) {
        this.seqnum = seqnum;
    }
    
    public void SetAck(short ack) {
        this.ack = ack;
    }
    
    public void SetAckBitfield(short ackbitfield) {
        this.ackbitfield = ackbitfield;
    }
    
    public void UpdateSendAttempt() {
        this.attempts++;
        this.sendtime = System.currentTimeMillis();
    }
}
