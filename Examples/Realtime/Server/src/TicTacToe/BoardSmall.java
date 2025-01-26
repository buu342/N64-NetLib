package TicTacToe;

public class BoardSmall {
    
    // Board state
    private int movecount;
    private Player winner;
    private Player moves[][];
    
    /**
     * A small 3x3 TicTacToe board
     */
    public BoardSmall() {
        this.winner = null;
        this.moves = new Player[3][3];
        this.movecount = 0;
    }
    
    /**
     * Make a move on the board
     * @param ply   The player who made the move
     * @param xpos  The x position of the move (from 0 to 2)
     * @param ypos  The y position of the move (from 0 to 2)
     */
    public boolean MakeMove(Player ply, int xpos, int ypos) {
        if (this.moves[xpos][ypos] != null)
            return false;
        this.moves[xpos][ypos] = ply;
        this.movecount++;
        this.CheckWinner();
        return true;
    }
    
    /**
     * Check if there's a winner in the game board
     */
    private void CheckWinner() {
        boolean haswinner;
        
        // Check rows
        for (int y=0; y<3; y++) {
            haswinner = true;
            if (this.moves[0][y] != null) {
                for (int x=1; x<3; x++) {
                    if (this.moves[x][y] != this.moves[x-1][y]) {
                        haswinner = false;
                        break;
                    }
                }
            }
            else
                haswinner = false;
            if (haswinner) {
                this.winner = this.moves[0][y];
                return;
            }
        }
        
        // Check columns
        for (int x=0; x<3; x++) {
            haswinner = true;
            if (this.moves[x][0] != null) {
                for (int y=1; y<3; y++) {
                    if (this.moves[x][y] != this.moves[x][y-1]) {
                        haswinner = false;
                        break;
                    }
                }
            }
            else
                haswinner = false;
            if (haswinner) {
                this.winner = this.moves[x][0];
                return;
            }
        }
        
        // Check first diagonal
        haswinner = true;
        if (this.moves[0][0] != null) {
            for (int x=1; x<3; x++) {
                if (this.moves[x][x] != this.moves[x-1][x-1]) {
                    haswinner = false;
                    break;
                }
            }
        }
        else
            haswinner = false;
        if (haswinner) {
            this.winner = this.moves[0][0];
            return;
        }

        // Check other diagonal
        haswinner = true;
        if (this.moves[2][0] != null) {
            for (int x=1; x<3; x++) {
                if (this.moves[2-x][x] != this.moves[2-(x-1)][x-1]) {
                    haswinner = false;
                    break;
                }
            }
        }
        else
            haswinner = false;
        if (haswinner) {
            this.winner = this.moves[2][0];
            return;
        }
    }
    
    /**
     * Get if the board has been completed
     * @return  Whether or not the board was completed
     */
    public boolean BoardFinished() {
        if (this.winner != null)
            return true;
        return (this.movecount == 9);
    }

    /**
     * Get the winner of this board
     * @return  The winning player, or null if there's a tie
     */
    public Player GetWinner() {
        return this.winner;
    }

    /**
     * Check which player made a move on this position of the board
     * @param x  The x position of the move (from 0 to 2)
     * @param y  The y position of the move (from 0 to 2)
     * @return  The player who made a move in this position, or null if the position is empty
     */
    public Player GetMove(int x, int y) {
        return this.moves[x][y];
    }

    /**
     * Represent this game board as a string
     * @return  The string representation of the board
     */
    public String toString() {
        String mystr = "";
        for (int y=0; y<3; y++) {
            for (int x=0; x<3; x++) {
                if (this.moves[x][y] == null)
                    mystr += " ";
                else if (this.moves[x][y].GetNumber() == 1)
                    mystr += "x";
                else if (this.moves[x][y].GetNumber() == 2)
                    mystr += "o";
            }
            mystr += "\n";
        }
        return mystr;
    }
}
