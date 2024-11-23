package NetLib;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

public class NetLibPacket {
    
    private static final int    PACKET_VERSION = 1;
    private static final String PACKET_HEADER = "NLP";
    private static final int    PACKET_MAXSIZE = 4096;
    
    private int version;
    private int type;
    private int flags;
    private int recipients;
    private int sender;
    private short seqnum;
    private short ack;
    private short ackbitfield;
    private short size;
    private byte data[];
    
    private NetLibPacket(int version, int type, int flags, int recipients, byte data[], short seqnum, short ack, short ackbitfield) {
        this.version = version;
        this.type = type;
        this.flags = flags;
        this.recipients = recipients;
        this.data = data;
        this.seqnum = seqnum;
        this.ack = ack;
        this.sender = 0;
        this.ackbitfield = ackbitfield;
        if (data != null)
            this.size = (short)data.length;
        else
            this.size = 0;
        if (this.size > PACKET_MAXSIZE)
            System.err.println("Packet size exceeds N64 library's capacity!");
    }
    
    public NetLibPacket(int type, byte data[]) {
        this.version = PACKET_VERSION;
        this.type = type;
        this.flags = 0;
        this.recipients = 0;
        this.sender = 0;
        this.seqnum = 0;
        this.ack = 0;
        this.ackbitfield = 0;
        this.data = data;
        if (data != null)
            this.size = (short)data.length;
        else
            this.size = 0;
        if (this.size > PACKET_MAXSIZE)
            System.err.println("Packet size exceeds N64 library's capacity!");
    }
    
    public String toString() {
        String mystr = "NetLib Packet\n";
        mystr += "    Version: " + this.version + "\n";
        mystr += "    Type: " + this.type + "\n";
        mystr += "    Recipients: " + Integer.toBinaryString(this.recipients) + "\n";
        if (this.sender != 0)
            mystr += "    Sender: " + this.sender + "\n";
        mystr += "    Data size: " + this.size + "\n";
        mystr += "    Data: \n";
        mystr += "        ";
        if (this.data != null)
            for (int i=0; i<this.data.length; i++)
                mystr += this.data[i] + " ";
        return mystr;
    }
    
    private static boolean CheckCString(byte[] data, String str) {
        int max = Math.min(str.length(), data.length);
        for (int i=0; i<max; i++) {
            char readbyte = (char) (((int) data[i]) & 0xFF);
            if (readbyte != str.charAt(i))
                return false;
        }
        return true;
    }
    
    public static boolean IsNetLibPacketHeader(byte[] data) {
        if (CheckCString(data, PACKET_HEADER))
            return true;
        return false;
    }
    
    static short getShort(byte[] arr) {
        return (short) ((0xff & arr[0]) << 8 | (0xff & arr[1]));
    }
    
    static int getInt(byte[] arr) {
        return ((arr[0] & 0xFF) << 24) | 
                ((arr[1] & 0xFF) << 16) | 
                ((arr[2] & 0xFF) << 8 ) | 
                ((arr[3] & 0xFF) << 0 );
    }

    
    static public NetLibPacket ReadPacket(byte[] pktdata) throws IOException {
        int version, type, flags, recipients;
        short seqnum, ack, ackbitfield;
        int size;
        byte[] data;
        ByteArrayInputStream dis = new ByteArrayInputStream(pktdata);
        
        // Get the packet header
        data = dis.readNBytes(PACKET_HEADER.length());
        if (!CheckCString(data, PACKET_HEADER)) {
            return null;
        }
        version = dis.read();

        // Get other data
        type = dis.read();
        flags = dis.read();
        seqnum = getShort(dis.readNBytes(2));
        ack = getShort(dis.readNBytes(2));
        ackbitfield = getShort(dis.readNBytes(2));
        recipients = getInt(dis.readNBytes(4));
        
        // Read the data
        size = getShort(dis.readNBytes(2));
        if (size > 0)
            data = dis.readNBytes(size);
        else
            data = null;
        dis.close();
        return new NetLibPacket(version, type, flags, recipients, data, seqnum, ack, ackbitfield);
    }
    
    /*
        NLP - 3 bytes
        Version - 1 byte
        Type - 1 byte
        Flags - 1 byte
        Sequence num - 2 bytes
        Last Ack - 2 bytes
        Ack bitfield - 2 bytes
        Recipients - 4 bytes
        Data size - 2 bytes
        Data - n bytes
     */
    
    public byte[] GetBytes() throws IOException {
        byte[] out;
        ByteBuffer buf = ByteBuffer.allocate(PACKET_MAXSIZE);
        buf.put(PACKET_HEADER.getBytes(StandardCharsets.US_ASCII), 0, PACKET_HEADER.length());
        buf.put((byte)this.version);
        buf.put((byte)this.type);
        buf.put((byte)this.flags);
        buf.putShort(this.seqnum);
        buf.putShort(this.ack);
        buf.putShort(this.ackbitfield);
        buf.putInt(this.recipients);
        buf.putShort(this.size);
        if (this.size > 0)
            buf.put(this.data, 0, this.data.length);
        out = new byte[buf.position()];
        buf.position(0);
        buf.get(out);
        return out;
    }
    
    public int GetVersion() {
        return this.version;
    }
    
    public int GetType() {
        return this.type;
    }
    
    public int GetFlags() {
        return this.flags;
    }
    
    public int GetRecipients() {
        return this.recipients;
    }
    
    public int GetSize() {
        return this.size;
    }
    
    public byte[] GetData() {
        return this.data;
    }
    
    public int GetSender() {
        return this.sender;
    }
    
    public short GetSequenceNumber() {
        return this.seqnum;
    }
    
    public void SetFlags(int flags) {
        this.flags = flags;
    }
    
    public void AddRecipient(int plynum) {
        this.recipients |= (1 << (plynum - 1));
    }
    
    public void SetSender(int plynum) {
        this.sender = plynum;
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

}