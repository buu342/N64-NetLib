# Master Server Application

This application gets pinged by servers in order to make them discoverable by clients using the NetLib Browser. It also allows users to download ROMs directly from the master server.

The master server will store the known servers in its database for 10 minutes. If the master server does not receive a heartbeat packet after that time elapses, the server is erased from its database.

By default, ROMs should be placed in a `roms` directory in the working directory of the program.

Unless you have a good reason to (like you want to host your own master server), you do not need to ever touch this application.