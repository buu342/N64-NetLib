package NetLib;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public class USBPacket {
    
    private static final String PACKET_HEADER = "DMA@";
    private static final int    PACKET_TYPE_NETLIB = 0x27;
    private static final String PACKET_TAIL = "CMPH";
    
    private int type;
    private int size;
    private byte data[];
    
    public USBPacket(int type, byte data[]) {
        this.type = type;
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

    static public USBPacket ReadPacket(DataInputStream dis) throws IOException {
        int size;
        byte[] data;
        
        // Get the packet header
        data = dis.readNBytes(4);
        if (!CheckCString(data, PACKET_HEADER)) {
            return null;
        }
        
        // Create the packet
        size = dis.readInt() & 0x00FFFFFF;
        data = dis.readNBytes(size);
        dis.readInt(); // CMPH
        return new USBPacket(PACKET_TYPE_NETLIB, data);
    }

    public void WritePacket(DataOutputStream dos) throws IOException {
        dos.write(PACKET_HEADER.getBytes(StandardCharsets.US_ASCII), 0, PACKET_HEADER.length());
        dos.writeInt((this.size & 0x00FFFFFF) | (this.type << 24));
        dos.write(this.data);
        dos.write(PACKET_TAIL.getBytes(StandardCharsets.US_ASCII), 0, PACKET_TAIL.length());
        dos.flush();
    }
    
    public int GetSize() {
        return this.size;
    }
    
    public byte[] GetData() {
        return this.data;
    }
}
