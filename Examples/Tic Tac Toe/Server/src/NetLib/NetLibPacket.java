package NetLib;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

public class NetLibPacket extends AbstractPacket {
    
    private static final int    PACKET_VERSION    = 1;
    private static final String PACKET_HEADER     = "NLP";
    
    private int type;
    private int recipients;
    private int sender;
    
    private NetLibPacket(int version, int type, byte data[], int flags, int recipients, short seqnum, short ack, short ackbitfield) {
        super(version, data, flags, seqnum, ack, ackbitfield);
        this.type = type;
        this.recipients = recipients;
        this.sender = 0;
    }
    
    public NetLibPacket(int type, byte data[], int flags) {
        this(PACKET_VERSION, type, data, flags, 0, (short)0, (short)0, (short)0);
    }
    
    public NetLibPacket(int type, byte data[]) {
        this(PACKET_VERSION, type, data, 0, 0, (short)0, (short)0, (short)0);
    }
    
    public String toString() {
        String mystr = "NetLib Packet\n";
        mystr += "    Version: " + this.version + "\n";
        mystr += "    Type: " + this.type + "\n";
        mystr += "    Sequence Number: " + this.seqnum + "\n";
        mystr += "    Ack: " + this.ack + "\n";
        mystr += "    AckField: " + Integer.toBinaryString(this.ackbitfield) + "\n";
        mystr += "    Recipients: " + Integer.toBinaryString(this.recipients) + "\n";
        if (this.sender != 0)
            mystr += "    Sender: " + this.sender + "\n";
        mystr += "    Data size: " + this.size + "\n";
        if (this.size > 0) {
            mystr += "    Data: \n";
            mystr += "        ";
            for (int i=0; i<this.data.length; i++)
                mystr += this.data[i] + " ";
        }
        return mystr;
    }
    
    public static boolean IsNetLibPacketHeader(byte[] data) {
        if (CheckCString(data, PACKET_HEADER))
            return true;
        return false;
    }
    
    public boolean IsAckBeat() {
        return this.type == 0;
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
        return new NetLibPacket(version, type, data, flags, recipients, seqnum, ack, ackbitfield);
    }
    
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
    
    public int GetType() {
        return this.type;
    }
    
    public int GetRecipients() {
        return this.recipients;
    }
    
    public int GetSender() {
        return this.sender;
    }
    
    public void AddRecipient(int plynum) {
        this.recipients |= (1 << (plynum - 1));
    }
    
    public void SetSender(int plynum) {
        this.sender = plynum;
    }
}