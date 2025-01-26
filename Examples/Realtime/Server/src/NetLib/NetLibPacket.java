package NetLib;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;

public class NetLibPacket extends AbstractPacket {

    // Constants
    private static final int    PACKET_VERSION    = 1;
    private static final String PACKET_HEADER     = "NLP";
    
    // Packet data
    private int type;
    private int recipients;
    private int sender;

    /**
     * A NetLib packet, used for game server <-> N64 communication
     * @param version      The version of the packet
     * @param type         The type of the packet
     * @param data         The packet data
     * @param flags        The flags to append to the packet
     * @param recipients   The target recipients for the packet
     * @param seqnum       The sequence number of the packet
     * @param ack          The sequence number of the last received packet
     * @param ackbitfield  The ack bitfield of the last received packets
     */
    private NetLibPacket(int version, int type, byte data[], int flags, int recipients, short seqnum, short ack, short ackbitfield) {
        super(version, data, flags, seqnum, ack, ackbitfield);
        this.type = type;
        this.recipients = recipients;
        this.sender = 0;
    }

    /**
     * A NetLib packet, used for game server <-> N64 communication
     * @param version      The version of the packet
     * @param data         The packet data
     * @param flags        The flags to append to the packet
     */
    public NetLibPacket(int type, byte data[], int flags) {
        this(PACKET_VERSION, type, data, flags, 0, (short)0, (short)0, (short)0);
    }

    /**
     * A NetLib packet, used for game server <-> N64 communication
     * @param version      The version of the packet
     * @param data         The packet data
     */
    public NetLibPacket(int type, byte data[]) {
        this(PACKET_VERSION, type, data, 0, 0, (short)0, (short)0, (short)0);
    }

    /**
     * Converts the packet into a string representation.
     * @return  The string representation of the packet
     */
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

    /**
     * Checks if the packet's header corresponds with one of a NetLib packet
     * @param data  The bytes to check if a NetLib packet exists within
     * @return  Whether the packet header corresponds with one of a NetLib packet 
     */
    public static boolean IsNetLibPacketHeader(byte[] data) {
        if (CheckCString(data, PACKET_HEADER))
            return true;
        return false;
    }

    /**
     * Checks if this packet is an Ack/Heartbeat packet
     * @return  Whether this packet is an Ack/Heartbeat packet
     */
    public boolean IsAckBeat() {
        return this.type == 0;
    }

    /**
     * Converts a byte array to a NetLib packet object
     * @param pktdata  The data to convert
     * @return  The converted NetLib packet, or null
     * @throws IOException                If an I/O error occurs
     * @throws BadPacketVersionException  If the packet is a higher version than supported
     */
    static public NetLibPacket ReadPacket(byte[] pktdata) throws IOException, BadPacketVersionException {
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
        if (version > PACKET_VERSION)
            throw new BadPacketVersionException(version);

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

    /**
     * Converts the NetLib packet into a set of raw bytes
     * @return  The NetLib packet converted into raw bytes
     */
    public byte[] GetBytes() {
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

    /**
     * Gets the type of this packet
     * @return  The type of the packet
     */
    public int GetType() {
        return this.type;
    }

    /**
     * Gets the recipients field of this packet
     * @return  The recipients field of the packet
     */
    public int GetRecipients() {
        return this.recipients;
    }

    /**
     * Gets which client sent this packet
     * @return  The number of the client that sent this packet
     */
    public int GetSender() {
        return this.sender;
    }

    /**
     * Adds a recipient to the recipient mask of this packet
     * @param plynum  The number of the client to send this packet to
     */
    public void AddRecipient(int plynum) {
        this.recipients |= (1 << (plynum - 1));
    }

    /**
     * Set the number of the client who sent this packet
     * @param plynum  The number of the client that sent this packet
     */
    public void SetSender(int plynum) {
        this.sender = plynum;
    }
}