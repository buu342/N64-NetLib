package NetLib;

public class N64Sever
{
	String name;
	String address;
	String ROM;
	int maxplayers;
	int curplayers;
	
	N64Sever(String name, int maxplayers, String address, String ROM)
	{
		this.name = name;
		this.maxplayers = maxplayers;
		this.curplayers = 0;
		this.address = address;
		this.ROM = ROM;
	}
}