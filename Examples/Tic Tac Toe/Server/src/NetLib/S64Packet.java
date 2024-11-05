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
    
    private S64Packet(int version, String type, int size, byte data[]) {
        this.version = version;
        this.type = type;
        this.size = size;
        this.data = data;
    }
    
    public S64Packet(String type, byte data[]) {
        this.version = PACKET_VERSION;
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

    static public S64Packet ReadPacket(DataInputStream dis) throws IOException {
        int version;
        int typesize;
        int size;
        byte[] data;
        String type = "";
        
        // Get the packet header
        data = dis.readNBytes(6);
        if (!CheckCString(data, "S64PKT")) {
            return null;
        }
        version = dis.readShort();
        typesize = Byte.toUnsignedInt(dis.readByte());
        data = dis.readNBytes(typesize);
        for (int i=0; i<typesize; i++)
            type += data[i];
        size = dis.readInt();
        data = dis.readNBytes(size);
        return new S64Packet(version, type, size, data);
    }
    
    public void WritePacket(DataOutputStream dos) throws IOException {
        dos.write(PACKET_HEADER.getBytes(StandardCharsets.US_ASCII));
        dos.writeShort(this.version);
        dos.write(new byte[]{(byte) this.type.length()});
        dos.write(this.type.getBytes(StandardCharsets.US_ASCII));
        dos.writeInt(this.size);
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
