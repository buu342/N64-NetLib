package TicTacToe;

public class BoardLarge {
    private int movecount;
    private BoardSmall boards[][];
    private BoardSmall forcedboard;
    private Player winner;
    
    /**
     * A large 9x9 Ultimate TicTacToe board
     */
    public BoardLarge() {
        this.winner = null;
        this.boards = new BoardSmall[3][3];
        this.forcedboard = null;
        this.movecount = 0;
        for (int y=0; y<3; y++)
            for (int x=0; x<3; x++)
                this.boards[x][y] = new BoardSmall();
    }
    
    /**
     * Make a move on the board
     * @param ply     The player who made the move
     * @param xboard  The x position of the small TicTacToe board on the large board (from 0 to 2)
     * @param yboard  The y position of the small TicTacToe board on the large board (from 0 to 2)
     * @param xpos    The x position of the move on the small board (from 0 to 2)
     * @param ypos    The y position of the move on the small board (from 0 to 2)
     */
    public boolean MakeMove(Player ply, int xboard, int yboard, int xpos, int ypos) {
        boolean wasvalid = false;
        if (xboard > 2 || yboard > 2 || xpos > 2 || ypos > 2)
            return false;
        
        // Check if the move was valid
        if (this.forcedboard == null || forcedboard == this.boards[xboard][yboard]) {
            wasvalid = this.boards[xboard][yboard].MakeMove(ply, xpos, ypos);
            this.CheckWinner();
            if (this.boards[xboard][yboard].BoardFinished())
                this.movecount++;
        }
        
        // If it was, check which board to force next
        if (wasvalid) {
            this.forcedboard = this.boards[xpos][ypos];
            if (this.forcedboard.BoardFinished())
                this.forcedboard = null;
        }
        return wasvalid;
    }
    
    /**
     * Check if there's a winner in the game board
     */
    private void CheckWinner() {
        boolean haswinner;
        
        // Check rows
        for (int y=0; y<3; y++) {
            haswinner = true;
            if (this.boards[0][y].GetWinner() != null) {
                for (int x=1; x<3; x++) {
                    if (this.boards[x][y].GetWinner() != this.boards[x-1][y].GetWinner()) {
                        haswinner = false;
                        break;
                    }
                }
            }
            else
                haswinner = false;
            if (haswinner) {
                this.winner = this.boards[0][y].GetWinner();
                return;
            }
        }
        
        // Check columns
        for (int x=0; x<3; x++) {
            haswinner = true;
            if (this.boards[x][0].GetWinner() != null) {
                for (int y=1; y<3; y++) {
                    if (this.boards[x][y].GetWinner() != this.boards[x][y-1].GetWinner()) {
                        haswinner = false;
                        break;
                    }
                }
            }
            else
                haswinner = false;
            if (haswinner) {
                this.winner = this.boards[x][0].GetWinner();
                return;
            }
        }
        
        // Check first diagonal
        haswinner = true;
        if (this.boards[0][0].GetWinner() != null) {
            for (int x=1; x<3; x++) {
                if (this.boards[x][x].GetWinner() != this.boards[x-1][x-1].GetWinner()) {
                    haswinner = false;
                    break;
                }
            }
        }
        else
            haswinner = false;
        if (haswinner) {
            this.winner = this.boards[0][0].GetWinner();
            return;
        }

        // Check other diagonal
        haswinner = true;
        if (this.boards[2][0].GetWinner() != null) {
            for (int x=1; x<3; x++) {
                if (this.boards[2-x][x].GetWinner() != this.boards[2-(x-1)][x-1].GetWinner()) {
                    haswinner = false;
                    break;
                }
            }
        }
        else
            haswinner = false;
        if (haswinner) {
            this.winner = this.boards[2][0].GetWinner();
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
     * Get the small board at the given position of the large board
     * @param x  The x position of the small board on the large board (from 0 to 2)
     * @param y  The y position of the small board on the large board (from 0 to 2)
     * @return  The small board in this coordinate
     */
    public BoardSmall GetBoard(int xpos, int ypos) {
        return this.boards[xpos][ypos];
    }

    /**
     * Get the number of the board where the next player is forced to play on
     * @return  The number of the board that the next player is forced to play on (from 1 to 9), or 0
     */
    public int GetForcedBoardNumber() {
        int boardnum = 1;
        if (this.forcedboard == null)
            return 0;
        for (int y=0; y<3; y++) {
            for (int x=0; x<3; x++) {
                if (this.forcedboard == this.boards[x][y])
                    return boardnum;
                boardnum++;
            }
        }
        return boardnum;
    }

    /**
     * Represent this game board as a string
     * @return  The string representation of the board
     */
    public String toString() {
        String mystr = "";
        for (int y=0; y<9; y++) {
            for (int x=0; x<9; x++) {
                if (!this.boards[x/3][y/3].BoardFinished()) {
                   Player move = this.boards[x/3][y/3].GetMove(x%3, y%3);
                   if (move == null)
                       mystr += " ";
                   else if (move.GetNumber() == 1)
                       mystr += "x";
                   else
                       mystr += "o";
                } else if (this.boards[x/3][y/3].GetWinner() == null) {
                    mystr += "T";
                } else if (this.boards[x/3][y/3].GetWinner().GetNumber() == 1) {
                    mystr += "X";
                } else if (this.boards[x/3][y/3].GetWinner().GetNumber() == 2) {
                    mystr += "O";   
                }
                if (x == 2 || x == 5)
                    mystr += "|";
            }
            if (y == 2 || y == 5) {
                mystr += "\n";
                for (int x=0; x<11; x++) {
                    if (x == 3 || x == 7)
                        mystr += "+";
                    else
                        mystr += "-";
                }
            }
            if (y != 8)
                mystr += "\n";
        }
        return mystr;
    }
}
