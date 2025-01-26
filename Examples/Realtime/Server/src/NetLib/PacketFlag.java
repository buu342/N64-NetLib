package NetLib;

public enum PacketFlag {
    FLAG_UNRELIABLE(0x01),
    FLAG_EXPLICITACK(0x02),
    ;

    // Int representation
    private int val;

    /**
     * Set a flag on the packet
     * @param val  The integer value of this flag
     */
    PacketFlag(int val) {
        this.val = val;
    }

    /**
     * Get the integer representation of this flag
     * @param val  The integer representation of this flag
     */
    public int GetInt() {
        return val;
    }
}