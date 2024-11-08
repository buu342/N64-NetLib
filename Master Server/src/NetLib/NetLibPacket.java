package NetLib;

import java.io.IOException;
import java.util.Arrays;

public class NetLibPacket {
    
    private static final int PACKET_VERSION = 1;
    
    private int version;
    private int id;
    private int size;
    private int to;
    private byte data[];
    
    private NetLibPacket(int version, int id, int to, byte data[]) {
        this.version = version;
        this.id = id;
        this.to = to;
        this.data = data;
        if (data != null)
            this.size = data.length;
        else
            this.size = 0;
    }
    
    public NetLibPacket(int id, int to, byte data[]) {
        this.version = PACKET_VERSION;
        this.id = id;
        this.to = to;
        this.data = data;
        if (data != null)
            this.size = data.length;
        else
            this.size = 0;
    }
    
    static public NetLibPacket ReadPacket(byte pkt[]) throws IOException {
        int version;
        int id;
        int to;
        byte[] data;
        
        // Get the packet header
        version = pkt[0];
        id = pkt[1];
        to = (pkt[4] << 24) | (pkt[5] << 16) | (pkt[6] << 8) | pkt[7];
        
        // Create the packet
        data = Arrays.copyOfRange(pkt, 8, pkt.length);
        return new NetLibPacket(version, id, to, data);
    }
    
    public int GetID() {
        return this.id;
    }
    
    public int GetVersion() {
        return this.version;
    }
    
    public int GetTo() {
        return this.to;
    }
    
    public int GetSize() {
        return this.size;
    }
    
    public byte[] GetData() {
        return this.data;
    }

}