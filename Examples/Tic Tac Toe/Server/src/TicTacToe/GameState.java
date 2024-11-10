package TicTacToe;

public enum GameState {
    GAMESTATE_LOBBY(0),
    GAMESTATE_READY(1),
    GAMESTATE_PLAYING(2),
    GAMESTATE_ENDED_WINNER_1(3),
    GAMESTATE_ENDED_WINNER_2(4),
    GAMESTATE_ENDED_TIE(5),
    GAMESTATE_ENDED_DISCONNECT(6),
    ;

    private int val;

    GameState(int numVal) {
        this.val = numVal;
    }

    public int GetInt() {
        return val;
    }
}