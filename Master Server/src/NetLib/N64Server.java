package NetLib;
import java.io.ByteArrayOutputStream;
import java.nio.ByteBuffer;

public class N64Server
{    
	private String name;
	private String address;
	private String ROM;
    private byte[] ROMHash;
    private int maxplayers;
	
	public N64Server(String name, int maxplayers, String address, String ROM, byte[] ROMHash)
	{
		this.name = name;
		this.maxplayers = maxplayers;
		this.address = address;
		this.ROM = ROM;
        this.ROMHash = ROMHash;
	}
	
	public byte[] toByteArray()
	{
		try {
			ByteArrayOutputStream bytes	= new ByteArrayOutputStream();
			bytes.write(ByteBuffer.allocate(4).putInt(this.name.length()).array());
			bytes.write(this.name.getBytes());
			bytes.write(ByteBuffer.allocate(4).putInt(this.maxplayers).array());
			bytes.write(ByteBuffer.allocate(4).putInt(this.address.length()).array());
			bytes.write(this.address.getBytes());
			bytes.write(ByteBuffer.allocate(4).putInt(this.ROM.length()).array());
			bytes.write(this.ROM.getBytes());
            bytes.write(ByteBuffer.allocate(4).putInt(this.ROMHash.length).array());
            bytes.write(this.ROMHash);
			return bytes.toByteArray();
		} catch (Exception e) {
            e.printStackTrace();
			return null;
		}
	}
}