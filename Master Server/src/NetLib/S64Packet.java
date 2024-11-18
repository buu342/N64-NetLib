package NetLib;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

public class S64Packet {
    
    private static final String PACKET_HEADER = "S64";
    private static final int PACKET_VERSION = 1;
    public static final int PACKET_MAXSIZE = 4096;

    private int version;
    private String type;
    private short size;
    private byte data[];
    private short seqnum;
    private short ack;
    private short ackbitfield;
    
    private S64Packet(int version, String type, byte data[], short seqnum, short ack, short ackbitfield) {
        this.version = version;
        this.type = type;
        this.data = data;
        this.seqnum = seqnum;
        this.ack = ack;
        this.ackbitfield = ackbitfield;
        if (data != null)
            this.size = (short)data.length;
        else
            this.size = 0;
    }
    
    public S64Packet(String type, byte data[]) {
        this.version = PACKET_VERSION;
        this.type = type;
        this.data = data;
        this.seqnum = 0;
        this.ack = 0;
        this.ackbitfield = 0;
        if (data != null)
            this.size = (short)data.length;
        else
            this.size = 0;
    }
    
    public String toString() {
        String mystr = "NetLib Packet\n";
        mystr += "    Version: " + this.version + "\n";
        mystr += "    Type: " + this.type + "\n";
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
    
    static public boolean IsS64PacketHeader(byte[] data) {
        if (CheckCString(data, PACKET_HEADER))
            return true;
        return false;
    }
    
    static short getShort(byte[] arr) {
        return (short) ((0xff & arr[0]) << 8 | (0xff & arr[1]));
    }

    static public S64Packet ReadPacket(byte[] pktdata) throws IOException {
        int version;
        int typesize;
        int size;
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
        return new S64Packet(version, type, data, seqnum, ack, ackbitfield);
    }
    
    public byte[] GetBytes() throws IOException {
        byte[] out;
        ByteBuffer buf = ByteBuffer.allocate(PACKET_MAXSIZE);
        buf.put(PACKET_HEADER.getBytes(StandardCharsets.US_ASCII), 0, PACKET_HEADER.length());
        buf.put((byte)this.version);
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
    
    public int GetVersion() {
        return this.version;
    }
    
    public String GetType() {
        return this.type;
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
