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