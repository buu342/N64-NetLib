package N64;

import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;

public class N64Server {
    private String address;
    private int    publicport;
    private String ROMName;
    private byte[] ROMHash;
    private String ROMHashStr;
    
    public N64Server(String address, int publicport, String ROMName, byte[] ROMHash) {
        this.address = address;
        this.publicport = publicport;
        this.ROMName = ROMName;
        this.ROMHash = ROMHash;
        this.ROMHashStr = N64ROM.BytesToHash(ROMHash);;
    }
    
    public int GetAddress() {
        return this.publicport;
    }
    
    public int GetPort() {
        return this.publicport;
    }
    
    public byte[] GetROMName() {
        return this.ROMHash;
    }
    
    public byte[] GetROMHash() {
        return this.ROMHash;
    }
    
    public String GetROMHashStr() {
        return this.ROMHashStr;
    }
    
    public byte[] toByteArray(boolean hasrom) {
        try {
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            String publicaddr = this.address + ":" + this.publicport;
            bytes.write(ByteBuffer.allocate(4).putInt(publicaddr.length()).array());
            bytes.write(publicaddr.getBytes());
            bytes.write(ByteBuffer.allocate(4).putInt(this.ROMName.length()).array());
            bytes.write(this.ROMName.getBytes());
            bytes.write(ByteBuffer.allocate(4).putInt(this.ROMHash.length).array());
            bytes.write(this.ROMHash);
            bytes.write((byte)(hasrom ? 1 : 0));
            return bytes.toByteArray();
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }
}