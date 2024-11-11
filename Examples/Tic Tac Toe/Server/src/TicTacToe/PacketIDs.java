package TicTacToe;

public enum PacketIDs {
    PACKETID_CLIENTCONNECT(0),
    PACKETID_PLAYERINFO(1),
    PACKETID_PLAYERDISCONNECT(2),
    PACKETID_SERVERFULL(3),
    PACKETID_HEARTBEAT(4),
    PACKETID_PLAYERREADY(5),
    PACKETID_GAMESTATECHANGE(6),
    PACKETID_PLAYERTURN(7),
    PACKETID_PLAYERMOVE(8),
    PACKETID_PLAYERCURSOR(9),
    PACKETID_BOARDCOMPLETED(10),
    ;

    private int val;

    PacketIDs(int numVal) {
        this.val = numVal;
    }

    public int GetInt() {
        return val;
    }
}