# NetLib 

A set of tools and libraries to connect your Nintendo 64 homebrew to the internet.

This repository is organized as such:
* `Client App` - This is the clientside app that users will need to find a server, upload the ROM via USB to the N64, and keep tethered for the communication to work.
* `Master Server` - This is the master server application which every server will connect to. This will store a live database of known servers, allowing the clientside app to discover said servers.