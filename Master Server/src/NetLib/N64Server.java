package NetLib;

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
}