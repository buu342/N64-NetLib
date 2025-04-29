# Realtime - Client

This folder contains a Libultra ROM for the client to run on their N64.

### Compiling the ROM
The ROM is available for Libultra users who are using either a WindowsXP machine or the ModernSDK. A compiled version of the ROM should be available in the [releases page](../../../../../releases).

#### Libultra

Simply run `makeme.bat`. 

#### Libultra with ModernSDK

Simply call `make`.

### Playing

Upon connecting, you will spawn as a colored rectangle in the game room. Use the control stick to move around.

By default, no clientside improvements are performed, so your game may feel very laggy, and your inputs unresponsive. Press L on the controller to enable Clientside Prediction, which will make your input feel instantly more responsive. However, your character may move erratically, so press R on the controller to enable clientside reconciliation. With both of these improvments, your movement should be very smooth and responsive. However, none of these improvements affects other objects/players. If you press Z on the controller, entity interpolation will be enabled, which makes everything else on the screen move just as smoothly as you.

### Implementation Notes

In order to reduce the traffic going through the USB slot, I made a few opimizations in the server and client code which complicates the writing of reconciliation and interpolation code. The optimizations are:

1) Player inputs are buffered for a few frames and then sent in one packet. In the client code, this is done at a rate of 15hz.
2) The server will not send object information back if the object's properties (like its position) has not changed this tick.

The second point is especially troublesome. When reconciling, it's important to check that the received acknowledgement packet actually updates the player's position, otherwise you will incorrectly reapply the packets to the wrong object position. Also, when interpolating, you will have gaps in your object's previous positions. You need to either repeat an object's position each tick on the client (which requires iterating through all objects every frame), or go through all the previous positions and fill in the missing ticks once a new value is received from the server. My implementation does the latter, and it doesn't really do it very accurately in order to save on CPU time.

You might have also noticed some slight jitter with the current reconciliation implementation. This is down to differences with the Java and C code, which results in slightly different floating point results (for instance, Java's `sqrt` function is a lot more accurate). This is why usually a game server will be running on very similar (if not the same) hardware and software that clients will.