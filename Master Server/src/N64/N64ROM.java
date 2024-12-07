package N64;

import java.security.MessageDigest;
import java.io.File;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.file.Files;

public class N64ROM {
	
	// ROM data
    private String name;
    private String path;
    private byte[] hash;
    private int size;
    
    /**
     * An object representing an N64 ROM
     * @param file  A file pointer to the ROM
     * @throws InvalidROMException  When the file is not a valid N64 ROM
     * @throws IOException          If an I/O error occurs
     */
    public N64ROM(File file) throws InvalidROMException, IOException {
        byte[] rombytes;
        
        // Validate the ROM header. No reason to support v64 ROMs.
        rombytes = Files.readAllBytes(file.toPath());
        if (!(rombytes[0] == (byte)0x80 && rombytes[1] == (byte)0x37 && rombytes[2] == (byte)0x12 && rombytes[3] == (byte)0x40))
            throw new InvalidROMException();
        
        // Assign the arguments to the attributes
        this.name = file.getName();
        this.path = file.getAbsolutePath();
        this.size = rombytes.length;
        this.hash = GetROMHash(file);
    }
    
    /**
     * Gets the ROM file's name
     * @return  The ROM file's name
     */
    public String GetName() {
        return this.name;
    }
    
    /**
     * Gets the path to the ROM file
     * @return  The path to the ROM file
     */
    public String GetPath() {
        return this.path;
    }
    
    /**
     * Gets the size of the ROM file
     * @return  The size of the ROM file
     */
    public int GetSize() {
        return this.size;
    }

    /**
     * Gets the SHA256 hash representation of the ROM file
     * @return  The SHA256 hash representation of the ROM file
     */
    public byte[] GetHash() {
        return this.hash;
    }

    /**
     * Gets the SHA256 hash representation of the ROM file as a string
     * @return  The SHA256 hash representation of the ROM file as a string
     */
    public String GetHashString() {
        return BytesToHash(this.hash);
    }

    /**
     * Creates the SHA256 hash representation of the ROM file
     * @param file  The file to get the hash of
     * @return  The SHA256 hash representation of the ROM file
     */
    public static byte[] GetROMHash(File file) {
        int count;
        byte[] buffer = new byte[8192];
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            BufferedInputStream bis = new BufferedInputStream(new FileInputStream(file));
            while ((count = bis.read(buffer)) > 0)
                digest.update(buffer, 0, count);
            bis.close();
            return digest.digest();
        } catch (Exception e) {
            System.err.println("Error when generating hash for '"+file.getName()+"'.");
            e.printStackTrace();
            return null;
        }
    }
    
    /**
     * Converts an array of bytes representing a hash into a string
     * @param bytes  The raw bytes to convert to a string
     * @return  The hash represented as a string
     */
    public static String BytesToHash(byte[] bytes) {
        final char[] HEX_ARRAY = "0123456789ABCDEF".toCharArray();
        char[] hexChars = new char[bytes.length * 2];
        for (int i=0; i<bytes.length; i++) {
            int v = bytes[i] & 0xFF;
            hexChars[i*2] = HEX_ARRAY[v >>> 4];
            hexChars[i*2+1] = HEX_ARRAY[v & 0x0F];
        }
        return new String(hexChars);
    }
}