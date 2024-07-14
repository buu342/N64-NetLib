package NetLib;

import java.security.MessageDigest;
import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.util.Base64;

public class N64ROM {
	private String name;
	private String path;
	private byte[] hash;
	private int size;
	
	public N64ROM(String name, String path) throws Exception {
	    byte[] buffer= new byte[8192];
	    byte[] hash;
	    int count;
	    MessageDigest digest = MessageDigest.getInstance("SHA-256");
	    BufferedInputStream bis = new BufferedInputStream(new FileInputStream(path + name));
	    
	    // Assign the arguments to the attributes
		this.name = name;
		this.path = path;
	    
		// Generate the sha256 hash, and assign the ROM size
	    while ((count = bis.read(buffer)) > 0) {
	        digest.update(buffer, 0, count);
	        this.size += count;
	    }
	    bis.close();
	    hash = digest.digest();
	    
	    // Assign the hash
	    this.hash = Base64.getEncoder().encode(hash);
	}
}
