package Realtime;

public enum PacketIDs {
    PACKETID_ACKBEAT(0), // Needs to be zero, as it's used internally by the UDP handler
    PACKETID_CLIENTCONNECT(1),
    PACKETID_PLAYERINFO(2),
    PACKETID_PLAYERDISCONNECT(3),
    PACKETID_SERVERFULL(4),
    PACKETID_CLOCKSYNC(5),
    PACKETID_DONECONNECT(6),
    ;

    // Int representation
    private int val;

    /**
     * Set a packet's type
     * @param val  The integer value of this packet's type
     */
    PacketIDs(int val) {
        this.val = val;
    }

    /**
     * Get the integer representation of this enum
     * @param val  The integer representation of this enum
     */
    public int GetInt() {
        return val;
    }
}