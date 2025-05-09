# Master Server Application

This application gets pinged by servers in order to make them discoverable by clients using the NetLib Browser. It also allows users to download ROMs directly from itself.

The master server will store the known servers in its database for 10 minutes. If the master server does not receive a heartbeat packet after that time elapses, the server is erased from its database.

By default, ROMs should be placed in a `roms` directory in the working directory of the program.

Unless you have a good reason to (like you want to host your own master server), you do not need to ever touch this application.

### Compiling the Master Server with Eclipse
Open the project in [Eclipse](https://www.eclipse.org). By default, Eclipse should just compile it as soon as you open the project.

You can export a `.jar` by right clicking the project in the Project Explorer, going to Export, and selecting Runnable JAR file.

### Compiling the Master Server with Ant
Simply call `ant -noinput -buildfile build.xml`

### Running the Master Server

In Eclipse, just press the green play button. You can optionally add program arguments by modifying the Run Configuration. 

If you are trying to run the JAR, then use the following command in a terminal:

```
java -jar <FileName.jar> <Program arguments>
```

By default, when you try launch the master server, it will use the `roms` folder in the current working directory to find ROMs that clients can download. You can change the ROM folder path with the `-dir` argument.

The master server will use a default port of 6464. You can change this with the `-port` command. It will also attempt to use UPnP to let you host the master server without port forwarding. You can disable UPnP with the `-noupnp` argument.

A list of arguments is available with the `-help` arguments.