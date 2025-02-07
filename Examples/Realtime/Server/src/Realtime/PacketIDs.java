package Realtime;

public enum PacketIDs {
    PACKETID_ACKBEAT(0), // Needs to be zero, as it's used internally by the UDP handler
    PACKETID_CLIENTCONNECT(1),
    PACKETID_SERVERFULL(2),
    PACKETID_CLOCKSYNC(3),
    PACKETID_DONESYNC(4),
    PACKETID_CLIENTINFO(5),
    PACKETID_PLAYERINFO(6),
    PACKETID_PLAYERDISCONNECT(7),
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