package NetLib;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

public class NetLibPacket {
    
    private static final int    PACKET_VERSION = 1;
    private static final String PACKET_HEADER = "PKT";
    
    private int version;
    private int id;
    private int size;
    private int recipients;
    private byte data[];
    
    private NetLibPacket(int version, int id, int recipients, byte data[]) {
        this.version = version;
        this.id = id;
        this.recipients = recipients;
        this.data = data;
        if (data != null)
            this.size = data.length;
        else
            this.size = 0;
    }
    
    public NetLibPacket(int id, byte data[]) {
        this.version = PACKET_VERSION;
        this.id = id;
        this.recipients = 0;
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
    
    static public NetLibPacket ReadPacket(DataInputStream dis) throws IOException {
        int version;
        int id;
        int to;
        int size;
        byte[] data;
        
        // Get the packet header
        data = dis.readNBytes(3);
        if (!CheckCString(data, PACKET_HEADER)) {
            return null;
        }
        version = dis.readNBytes(1)[0];
        
        // Get the ID, size, and recipients
        size = dis.readInt();
        id = size >> 24;
        size &= 0x00FFFFFF;
        data = dis.readNBytes(4);
        to = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
        
        // Create the packet
        data = dis.readNBytes(size);
        return new NetLibPacket(version, id, to, data);
    }
    
    public void WritePacket(DataOutputStream dos) throws IOException {
        dos.write(PACKET_HEADER.getBytes(StandardCharsets.US_ASCII), 0, PACKET_HEADER.length());
        dos.write(new byte[]{(byte)this.version});
        dos.writeInt((this.size & 0x00FFFFFF) | (this.id << 24));
        dos.writeInt(this.recipients);
        dos.write(this.data);
        dos.flush();
    }
    
    public void AddRecipient(int plynum) {
        this.recipients |= (1 << (plynum - 1));
    }
    
    public int GetID() {
        return this.id;
    }
    
    public int GetVersion() {
        return this.version;
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

}