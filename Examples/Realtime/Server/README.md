# Realtime - Server

This folder contains the server implementation of the realtime example.

### Compiling the Server with Eclipse
Open the project in [Eclipse](https://www.eclipse.org). By default, Eclipse should just compile it as soon as you open the project.

You can export a `.jar` by right clicking the project in the Project Explorer, going to Export, and selecting Runnable JAR file.

### Compiling the Server with Ant
Simply call `ant -noinput -buildfile build.xml`

### Running the Server

In Eclipse, just press the green play button as soon as you have set up a run configuration to provide the server with arguments (like the server name).

If you are trying to run the JAR, then use the following command in a terminal:

```
java -jar <FileName.jar> <Program arguments>
```

By default, when you try launch the server, it will assume you want to register it to the Master Server. Because of this, you are required to both provide a name for the server (using the `-name` argument) and the ROM which connecting players need (using the `-rom` argument). If you don't wish to register the server, use `-noregister`. You can also use the `-master` command to provide a different Master Server to register the server to.

The server will use a default port of 6460. You can change this with the `-port` command. It will also attempt to use UPnP to let you host a server without port forwarding. You can disable UPnP with the `-noupnp` argument.

A list of arguments is available with the `-help` arguments.

Upon launching the server, a preview window that shows the current state of the game world will appear. Closing this window will stop the server.