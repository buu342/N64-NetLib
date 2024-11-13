package NetLib;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public class S64Packet {
    
    private static final String PACKET_HEADER = "S64PKT";
    private static final int PACKET_VERSION = 1;

    private int version;
    private String type;
    private int size;
    private byte data[];
    
    private S64Packet(int version, String type, byte data[]) {
        this.version = version;
        this.type = type;
        this.data = data;
        if (data != null)
            this.size = data.length;
        else
            this.size = 0;
    }
    
    public S64Packet(String type, byte data[]) {
        this.version = PACKET_VERSION;
        this.type = type;
        this.data = data;
        if (data != null)
            this.size = data.length;
        else
            this.size = 0;
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

    static public S64Packet ReadPacket(DataInputStream dis, boolean skipheader) throws IOException {
        int version;
        int typesize;
        int size;
        byte[] data;
        String type = "";
        
        // Get the packet header
        if (!skipheader) {
            data = dis.readNBytes(PACKET_HEADER.length());
            if (!CheckCString(data, PACKET_HEADER)) {
                return null;
            }
        }
        version = dis.readShort();
        typesize = Byte.toUnsignedInt(dis.readByte());
        data = dis.readNBytes(typesize);
        for (int i=0; i<typesize; i++)
            type += (char)data[i];
        size = dis.readInt();
        if (size > 0)
            data = dis.readNBytes(size);
        else
            data = null;
        return new S64Packet(version, type, data);
    }

    static public S64Packet ReadPacket(DataInputStream dis) throws IOException {
        byte[] data = dis.readNBytes(PACKET_HEADER.length());
        if (!CheckCString(data, PACKET_HEADER)) {
            return null;
        }
        return S64Packet.ReadPacket(dis, true);
    }
    
    public void WritePacket(DataOutputStream dos) throws IOException {
        dos.write(PACKET_HEADER.getBytes(StandardCharsets.US_ASCII), 0, PACKET_HEADER.length());
        dos.writeShort(this.version);
        dos.write(new byte[]{(byte) this.type.length()});
        dos.write(this.type.getBytes(StandardCharsets.US_ASCII));
        dos.writeInt(this.size);
        if (this.size > 0)
            dos.write(this.data);
        dos.flush();
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
}
