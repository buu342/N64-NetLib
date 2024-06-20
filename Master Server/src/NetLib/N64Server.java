package NetLib;
import java.io.ByteArrayOutputStream;

public class N64Server
{    
	private String name;
	private String address;
	private String ROM;
	private int maxplayers;
	private int curplayers;
	
	public N64Server(String name, int maxplayers, String address, String ROM)
	{
		this.name = name;
		this.maxplayers = maxplayers;
		this.curplayers = 0;
		this.address = address;
		this.ROM = ROM;
	}
	
	public byte[] toByteArray()
	{
		try {
			ByteArrayOutputStream bytes	= new ByteArrayOutputStream();
			bytes.write(this.name.getBytes());
			bytes.write(this.curplayers);
			bytes.write(this.maxplayers);
			bytes.write(this.address.getBytes());
			bytes.write(this.ROM.getBytes());
			return bytes.toByteArray();
		} catch (Exception e) {
			System.out.println(e);
			return null;
		}
	}
}