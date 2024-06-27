# NetLib 

A set of tools and libraries to connect your Nintendo 64 homebrew to the internet.

This repository is organized as such:
* `Client App` - This is the clientside app that users will need to find a server, upload the ROM via USB to the N64, and keep tethered for the communication to work.
* `N64 Library` - A library for use with N64 homebrew that allows some basic networking functionality.
* `Examples` - Example ROM and server applications that showcase the library in action.
* `Master Server` - This is the master server application which every server will connect to. This will store a live database of known servers, allowing the clientside app to discover said servers.

### Compilation

### FAQ

### Security

This project aims to keep the system extensible (IE anyone can make their own custom server for people to connect to), but safe to use. To ensure this, the following restrictions are in place:
* The official Client App will only download ROMs directly from the master server. This ensures that only vetted ROMs are allowed, and prevents piracy via distribution of modified commercial ROMs. Support for patches is not currently planned by me, but can be added in a PR.
* The official Client App only serves as a packet distributor, and will not handle UNFLoader packets such as debug prints.

Of course, I share no responsibility for the use of unofficial Client Apps. 