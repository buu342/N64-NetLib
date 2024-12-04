package NetLib;

public enum PacketFlag {
    FLAG_UNRELIABLE(0x01),
    FLAG_EXPLICITACK(0x02),
    ;

    private int val;

    PacketFlag(int numVal) {
        this.val = numVal;
    }

    public int GetInt() {
        return val;
    }
}