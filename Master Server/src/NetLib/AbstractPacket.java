package NetLib;

public abstract class AbstractPacket {
    
    // Constants
    public static final int    PACKET_MAXSIZE    = 4096;
    protected static final int PACKET_MAXSEQNUM  = 0xFFFF;

    // Packet data
    protected int version;
    protected int flags;
    protected short seqnum;
    protected short ack;
    protected short ackbitfield;
    protected short size;
    protected byte data[];
    
    // Extra data
    protected long sendtime;
    protected int attempts;

    /**
     * An abstract class that represents a packet for network communication.
     * Not to be used directly.
     * @param version      The version of the packet
     * @param data         The packet data
     * @param flags        The flags to append to the packet
     * @param seqnum       The sequence number of the packet
     * @param ack          The sequence number of the last received packet
     * @param ackbitfield  The ack bitfield of the last received packets
     */
    public AbstractPacket(int version, byte data[], int flags, short seqnum, short ack, short ackbitfield) {
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

    /**
     * An abstract class that represents a packet for network communication.
     * Not to be used directly.
     * @param version  The version of the packet
     * @param data     The packet data
     * @param flags    The flags to append to the packet
     */
    public AbstractPacket(int version, byte data[], int flags) {
        this(version, data, flags, (short)0, (short)0, (short)0);
    }

    /**
     * An abstract class that represents a packet for network communication.
     * Not to be used directly.
     * @param version  The version of the packet
     * @param data     The packet data
     */
    public AbstractPacket(int version, byte data[]) {
        this(version, data, 0, (short)0, (short)0, (short)0);
    }

    /**
     * Converts the packet into a string representation.
     * @return  The string representation of the packet
     */
    abstract public String toString();
    
    /**
     * Checks if a sequence number is larger than another, handling rollover
     * @param s1  The first sequence number
     * @param s2  The second sequence number
     * @return  Whether the first sequence number is larger
     */
    protected static boolean SequenceGreaterThan(int s1, int s2) {
        return ((s1 > s2) && (s1 - s2 <= ((PACKET_MAXSEQNUM/2)+1))) || ((s1 < s2) && (s2 - s1 > ((PACKET_MAXSEQNUM/2)+1)));
    }

    /**
     * Increments a sequence number, handling rollover
     * @param seq  The sequence number to increment
     * @return  The incremented number
     */
    protected static int SequenceIncrement(int seq) {
        return (seq + 1) % (PACKET_MAXSEQNUM + 1);
    }

    /**
     * Checks the delta between two sequence numbers, handling rollover
     * @param s1  The first sequence number
     * @param s2  The second sequence number
     * @return The numerical difference between two sequence numbers
     */
    protected static int SequenceDelta(int s1, int s2) {
        int delta = (s1 - s2);
        if (delta < 0)
            delta += PACKET_MAXSEQNUM+1;
        return delta;
    }

    /**
     * Checks if the C string data in the byte array matches a given Java string
     * @param data  The C string as a byte array
     * @param str   The java string to compare it to
     * @return Whether the string matches or not
     */
    protected static boolean CheckCString(byte[] data, String str) {
        int max = Math.min(str.length(), data.length);
        for (int i=0; i<max; i++) {
            char readbyte = (char) (((int) data[i]) & 0xFF);
            if (readbyte != str.charAt(i))
                return false;
        }
        return true;
    }

    /**
     * Retrieves a short from a byte array
     * @param arr  The array with the short
     * @return The short extracted from the byte array
     */
    protected static short getShort(byte[] arr) {
        return (short) ((0xff & arr[0]) << 8 | (0xff & arr[1]));
    }

    /**
     * Retrieves an int from a byte array
     * @param arr  The array with the int
     * @return The int extracted from the byte array
     */
    protected static int getInt(byte[] arr) {
        return ((arr[0] & 0xFF) << 24) | 
                ((arr[1] & 0xFF) << 16) | 
                ((arr[2] & 0xFF) << 8 ) | 
                ((arr[3] & 0xFF) << 0 );
    }

    /**
     * Creates a byte array representation of this packet
     * @return The byte array representation of this packet
     */
    abstract public byte[] GetBytes();
    
    /**
     * Retrieves the packet's version
     * @return The packet's version
     */
    public int GetVersion() {
        return this.version;
    }

    /**
     * Retrieves the packet's size
     * @return The packet's size
     */
    public int GetSize() {
        return this.size;
    }

    /**
     * Retrieves the packet's data
     * @return The packet's data
     */
    public byte[] GetData() {
        return this.data;
    }

    /**
     * Retrieves the packet's sequence number
     * @return The packet's sequence number
     */
    public short GetSequenceNumber() {
        return this.seqnum;
    }
    
    /**
     * Checks if this packet is acking a specific sequence number
     * @param number  The sequence number to check
     * @return Whether the given number is acked by this packet
     */
    public boolean IsAcked(short number) {
        if (this.ack == number)
            return true;
        return ((this.ackbitfield & (1 << (SequenceDelta(this.ack, number) - 1))) != 0);
    }

    /**
     * Retrieves the packet's flags
     * @return The packet's flags
     */
    public int GetFlags() {
        return this.flags;
    }
    
    /**
     * Retrieves the time elapsed (in milliseconds) since this packet was last sent
     * @return The time since the packet was sent (in milliseconds)
     */
    public long GetSendTime() {
        return System.currentTimeMillis() - this.sendtime;
    }
    
    /**
     * Retrieves the number of times this packet was attempted to be sent
     * @return The number of send attempts
     */
    public int GetSendAttempts() {
        return this.attempts;
    }
    
    /**
     * Checks if this packet is an Ack/Heartbeat packet
     * @return Whether this packet is an Ack/Heartbeat packet
     */
    abstract public boolean IsAckBeat();
    
    /**
     * Enables a set of flags on the packet
     * @param flags  The flags to enable
     */
    public void EnableFlags(int flags) {
        this.flags |= flags;
    }
    
    /**
     * Sets this packet's sequence number
     * @param seqnum  The sequence number to set
     */
    public void SetSequenceNumber(short seqnum) {
        this.seqnum = seqnum;
    }
    
    /**
     * Sets this packet's last ack number
     * @param ack  The last ack number
     */
    public void SetAck(short ack) {
        this.ack = ack;
    }
    
    /**
     * Sets this packet's ack bitfield
     * @param ack  The ack bitfield
     */
    public void SetAckBitfield(short ackbitfield) {
        this.ackbitfield = ackbitfield;
    }
    
    /**
     * Marks an attempt to send the packet
     */
    public void UpdateSendAttempt() {
        this.attempts++;
        this.sendtime = System.currentTimeMillis();
    }
}
