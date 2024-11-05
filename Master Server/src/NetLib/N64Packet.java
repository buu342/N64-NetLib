package NetLib;

import java.io.DataInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class N64Packet {
    
    private int version;
    private int size;
    private byte data[];
    
    private N64Packet(int version, int size, byte data[]) {
        this.version = version;
        this.size = size;
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
        ByteBuffer bb;
        byte[] data;
        
        // Get the packet header
        data = dis.readNBytes(6);
        if (!CheckCString(data, "N64PKT")) {
            return null;
        }
        
        // Get the packet version
        data = dis.readNBytes(2);
        bb = ByteBuffer.wrap(data);
        version = bb.getShort();
        
        // Get the packet size
        data = dis.readNBytes(4);
        bb = ByteBuffer.wrap(data);
        size = bb.getInt();
        
        // Create the packet
        data = dis.readNBytes(size);
        return new N64Packet(version, size, data);
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
