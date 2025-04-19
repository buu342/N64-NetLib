# Example Servers

This folder contains a list of example games, showing how to use the NetLib stack. The client ROMs were all written in Libultra, with Libdragon ports planned for the future. The servers were all written in Java using [Eclipse](https://www.eclipse.org). 

You do not need to make your game servers in Java, I just chose this language because it has very strong networking features out of the box. In fact I would probably recommend you try to use the same language for both your client and server, since otherwise you are going to be writing your game twice. Also, when it comes to realtime games, subtleties in the different languages could result in some unexpected behaviour.

### Credits

* The servers use [WaifUPnP](https://github.com/adolfintel/WaifUPnP) for UPnP