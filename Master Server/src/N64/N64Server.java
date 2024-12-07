package N64;

import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;

public class N64Server {
    
    // Server data
    private String address;
    private int    publicport;
    private String ROMName;
    private byte[] ROMHash;
    private String ROMHashStr;
    private long lastmessage;

    /**
     * An object representing a game server
     * @param address     The address of this server
     * @param publicport  The port players can use to connect to the server
     * @param ROMName     The name of the ROM used by the server
     * @param ROMHash     The hash of the ROM used by the server
     */
    public N64Server(String address, int publicport, String ROMName, byte[] ROMHash) {
        this.address = address;
        this.publicport = publicport;
        this.ROMName = ROMName;
        this.ROMHash = ROMHash;
        this.ROMHashStr = N64ROM.BytesToHash(ROMHash);
        this.lastmessage = System.currentTimeMillis();
    }

    /**
     * Gets the address of the server
     * @return  The server's address
     */
    public int GetAddress() {
        return this.publicport;
    }

    /**
     * Gets the port of the server
     * @return  The server's port
     */
    public int GetPort() {
        return this.publicport;
    }
    
    /**
     * Gets the ROM file's name
     * @return  The ROM file's name
     */
    public byte[] GetROMName() {
        return this.ROMHash;
    }

    /**
     * Gets the SHA256 hash representation of the ROM file
     * @return  The SHA256 hash representation of the ROM file
     */
    public byte[] GetROMHash() {
        return this.ROMHash;
    }

    /**
     * Gets the SHA256 hash representation of the ROM file as a string
     * @return  The SHA256 hash representation of the ROM file as a string
     */
    public String GetROMHashStr() {
        return this.ROMHashStr;
    }

    /**
     * Gets the last time this server talked to us
     * @return  The time (in milliseconds) when the server last talked to us
     */
    public long GetLastInteractionTime() {
        return this.lastmessage;
    }

    /**
     * Packs all the server data into a byte array
     * @param hasrom  Whether the master server has this server's ROM available for download
     * @return  The server represented as a byte array
     */
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

    /**
     * Updates the last time this server talked to us
     */
    public void UpdateLastInteractionTime() {
        this.lastmessage = System.currentTimeMillis();
    }
}