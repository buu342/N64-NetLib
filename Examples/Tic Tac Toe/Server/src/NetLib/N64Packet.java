package NetLib;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public class N64Packet {
    
    private static final String PACKET_HEADER = "N64PKT";
    private static final int PACKET_VERSION = 1;
    
    private int version;
    private int size;
    private byte data[];
    
    private N64Packet(int version, int size, byte data[]) {
        this.version = version;
        this.size = size;
        this.data = data;
    }
    
    public N64Packet(byte data[]) {
        this.version = PACKET_VERSION;
        this.size = data.length;
        this.data = data;
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

    static public N64Packet ReadPacket(DataInputStream dis) throws IOException {
        int version;
        int size;
        byte[] data;
        
        // Get the packet header
        data = dis.readNBytes(6);
        if (!CheckCString(data, PACKET_HEADER)) {
            return null;
        }
        
        // Create the packet
        version = dis.readShort();
        size = dis.readInt();
        data = dis.readNBytes(size);
        return new N64Packet(version, size, data);
    }

    public void WritePacket(DataOutputStream dos) throws IOException {
        dos.write(PACKET_HEADER.getBytes(StandardCharsets.US_ASCII), 0, PACKET_HEADER.length());
        dos.writeShort(this.version);
        dos.writeInt(this.size);
        dos.write(this.data);
        dos.flush();
    }
    
    public int GetVersion() {
        return this.version;
    }
    
    public int GetSize() {
        return this.size;
    }
    
    public byte[] GetData() {
        return this.data;
    }
}
