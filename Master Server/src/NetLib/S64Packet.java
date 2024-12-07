package NetLib;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

public class S64Packet extends AbstractPacket {
    
    private static final String PACKET_HEADER     = "S64";
    private static final int    PACKET_VERSION    = 1;

    private String type;
    
    private S64Packet(int version, String type, byte data[], int flags, short seqnum, short ack, short ackbitfield) {
        super(version, data, flags, seqnum, ack, ackbitfield);
        this.type = type;
    }
    
    public S64Packet(String type, byte data[], int flags) {
        this(PACKET_VERSION, type, data, flags, (short)0, (short)0, (short)0);
    }
    
    public S64Packet(String type, byte data[]) {
        this(PACKET_VERSION, type, data, 0, (short)0, (short)0, (short)0);
    }
    
    public String toString() {
        String mystr = "S64Packet Packet\n";
        mystr += "    Version: " + this.version + "\n";
        mystr += "    Type: " + this.type + "\n";
        mystr += "    Sequence Number: " + this.seqnum + "\n";
        mystr += "    Ack: " + this.ack + "\n";
        mystr += "    AckField: " + Integer.toBinaryString(this.ackbitfield) + "\n";
        mystr += "    Data size: " + this.size + "\n";
        if (this.size > 0) {
            mystr += "    Data: \n";
            mystr += "        ";
            for (int i=0; i<this.data.length; i++)
                mystr += this.data[i] + " ";
        }
        return mystr;
    }
    
    static public boolean IsS64PacketHeader(byte[] data) {
        if (CheckCString(data, PACKET_HEADER))
            return true;
        return false;
    }
    
    public boolean IsAckBeat() {
        return this.type == "ACK";
    }

    static public S64Packet ReadPacket(byte[] pktdata) throws IOException {
        int version;
        int typesize;
        int size;
        int flags;
        short seqnum;
        short ack;
        short ackbitfield;
        byte[] data;
        String type = "";
        ByteArrayInputStream dis = new ByteArrayInputStream(pktdata);
        
        // Get the packet header
        data = dis.readNBytes(PACKET_HEADER.length());
        if (!CheckCString(data, PACKET_HEADER)) {
            return null;
        }
        version = dis.read();
        flags = dis.read();

        // Get other data
        seqnum = getShort(dis.readNBytes(2));
        ack = getShort(dis.readNBytes(2));
        ackbitfield = getShort(dis.readNBytes(2));
        typesize = dis.read();
        for (int i=0; i<typesize; i++)
            type += (char)dis.read();
        
        // Read the data
        size = getShort(dis.readNBytes(2));
        if (size > 0)
            data = dis.readNBytes(size);
        else
            data = null;
        dis.close();
        return new S64Packet(version, type, data, flags, seqnum, ack, ackbitfield);
    }
    
    public byte[] GetBytes()  {
        byte[] out;
        ByteBuffer buf = ByteBuffer.allocate(PACKET_MAXSIZE);
        buf.put(PACKET_HEADER.getBytes(StandardCharsets.US_ASCII), 0, PACKET_HEADER.length());
        buf.put((byte)this.version);
        buf.put((byte)this.flags);
        buf.putShort(this.seqnum);
        buf.putShort(this.ack);
        buf.putShort(this.ackbitfield);
        buf.put((byte)this.type.length());
        buf.put(this.type.getBytes(StandardCharsets.US_ASCII), 0, this.type.length());
        buf.putShort(this.size);
        if (this.size > 0)
            buf.put(this.data, 0, this.data.length);
        out = new byte[buf.position()];
        buf.position(0);
        buf.get(out);
        return out;
    }
    
    public String GetType() {
        return this.type;
    }
}
