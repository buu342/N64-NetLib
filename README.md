# N64 Networking Library 

A set of tools and libraries to connect your Nintendo 64 homebrew to the internet for multiplayer.

This repository is organized as such:
* `Client App` - This is the clientside app that users will need to find a server, upload the ROM via USB to the N64, and keep tethered for the communication to work.
* `N64 Library` - A library for use with N64 homebrew that allows some basic networking functionality.
* `Examples` - Example ROM and server applications that showcase the library in action.
* `Master Server` - This is the master server application which every server will connect to. This will store a live database of known servers, allowing the clientside app to discover said servers.

### Compilation

### FAQ

**Q: What flashcarts are supported?**</br>
**A:** [Whatever flashcarts UNFLoader supports](https://github.com/buu342/N64-UNFLoader?tab=readme-ov-file#unfloader), as this project uses UNFLoader's libraries for USB communication.

**Q: Do I need to register my server with the master server in order for friends to join?**</br>
**A:** Nope. You can manually connect to known server addresses in the browser application. You will need to have the ROM that the server is using in order to be able to join it, however.

**Q: Does the library work for LAN or P2P connections?**</br>
**A:** In theory, yes. I haven't tried it though, and I haven't designed the browser application for that in mind.

**Q: Can I make this work with X retail game?**</br>
**A:** If you have the source code for said game, sure. But even then, such a thing would likely require a massive rewrite of the game's engine, as it very likely wasn't designed with networking in mind. In real life, game networking is *much more complex than just "sending a player's controller input over the wire"*. See [this article](https://developer.valvesoftware.com/wiki/Source_Multiplayer_Networking) for more information.

**Q: How can I get my ROM uploaded to the official master server?**</br>
**A:** Get in touch! Create a GitHub issue or send me an email.

### Security

This project aims to keep the system extensible (IE anyone can make their own custom server for people to connect to), but safe to use. To ensure this, the following restrictions are in place:
* The official Client App will only download ROMs directly from the master server. This ensures that only vetted ROMs are allowed, and prevents piracy via distribution of modified commercial ROMs. Support for patches is not currently planned by me, but can be added in a PR.
* The official Client App only serves as a packet distributor, and will not handle most UNFLoader packets outside of debug prints.

Of course, I share no responsibility for the use of unofficial Client Apps. 

One minor flaw currently present in the server code is that there is no ban system in place, so there is currently no way to ignore clients who keep unecessarily pinging the server. This is a minor thing (considering how niche of a project this is) and I intend to fix it in the future.