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

    // Int representation
    private int val;

    /**
     * Set the game state
     * @param val  The integer value of the game state
     */
    GameState(int val) {
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