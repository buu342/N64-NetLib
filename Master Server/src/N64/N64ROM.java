package N64;

import java.security.MessageDigest;
import java.io.File;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.nio.file.Files;

public class N64ROM {
    private String name;
    private String path;
    private byte[] hash;
    private int size;
    
    public N64ROM(File file) throws Exception {
        byte[] rombytes;
        
        // Validate the ROM header. No reason to support v64 ROMs.
        rombytes = Files.readAllBytes(file.toPath());
        if (!(rombytes[0] == (byte)0x80 && rombytes[1] == (byte)0x37 && rombytes[2] == (byte)0x12 && rombytes[3] == (byte)0x40))
            throw new Exception();
        
        // Assign the arguments to the attributes
        this.name = file.getName();
        this.path = file.getAbsolutePath();
        this.size = rombytes.length;
        this.hash = GetROMHash(file);
    }
    
    public String GetName() {
        return this.name;
    }
    
    public String GetPath() {
        return this.path;
    }
    
    public int GetSize() {
        return this.size;
    }
    
    public byte[] GetHash() {
        return this.hash;
    }
    
    public String GetHashString() {
        return BytesToHash(this.hash);
    }
    
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
