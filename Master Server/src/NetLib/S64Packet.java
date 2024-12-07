package NetLib;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

public class S64Packet extends AbstractPacket {

    // Constants
    private static final String PACKET_HEADER     = "S64";
    private static final int    PACKET_VERSION    = 1;

    // Packet data
    private String type;

    /**
     * An S64 packet, used for game server <-> master server <-> client communication
     * @param version      The version of the packet
     * @param type         The type of the packet
     * @param data         The packet data
     * @param flags        The flags to append to the packet
     * @param recipients   The target recipients for the packet
     * @param seqnum       The sequence number of the packet
     * @param ack          The sequence number of the last received packet
     * @param ackbitfield  The ack bitfield of the last received packets
     */
    private S64Packet(int version, String type, byte data[], int flags, short seqnum, short ack, short ackbitfield) {
        super(version, data, flags, seqnum, ack, ackbitfield);
        this.type = type;
    }

    /**
     * An S64 packet, used for game server <-> master server <-> client communication
     * @param type         The type of the packet
     * @param data         The packet data
     * @param flags        The flags to append to the packet
     */
    public S64Packet(String type, byte data[], int flags) {
        this(PACKET_VERSION, type, data, flags, (short)0, (short)0, (short)0);
    }

    /**
     * An S64 packet, used for game server <-> master server <-> client communication
     * @param type         The type of the packet
     * @param data         The packet data
     */
    public S64Packet(String type, byte data[]) {
        this(PACKET_VERSION, type, data, 0, (short)0, (short)0, (short)0);
    }

    /**
     * Converts the packet into a string representation.
     * @return  The string representation of the packet
     */
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

    /**
     * Checks if the packet's header corresponds with one of an S64 packet
     * @param data  The bytes to check if a NetLib packet exists within
     * @return  Whether the packet header corresponds with one of an S64 packet 
     */
    static public boolean IsS64PacketHeader(byte[] data) {
        if (CheckCString(data, PACKET_HEADER))
            return true;
        return false;
    }

    /**
     * Checks if this packet is an Ack/Heartbeat packet
     * @return  Whether this packet is an Ack/Heartbeat packet
     */
    public boolean IsAckBeat() {
        return this.type == "ACK";
    }

    /**
     * Converts a byte array to an S64 packet object
     * @param pktdata  The data to convert
     * @return  The converted S64 packet, or null
     */
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

    /**
     * Converts the NetLib packet into a set of raw bytes
     * @return  The NetLib packet converted into raw bytes
     */
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

    /**
     * Gets the type of this packet
     * @return  The type of the packet
     */
    public String GetType() {
        return this.type;
    }
}
