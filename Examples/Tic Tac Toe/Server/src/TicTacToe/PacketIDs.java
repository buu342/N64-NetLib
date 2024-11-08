package TicTacToe;

public enum PacketIDs {
    PACKETID_CLIENTCONNECT(0),
    PACKETID_PLAYERINFO(1),
    PACKETID_PLAYERDISCONNECT(2),
    ;

    private int val;

    PacketIDs(int numVal) {
        this.val = numVal;
    }

    public int GetInt() {
        return val;
    }
}