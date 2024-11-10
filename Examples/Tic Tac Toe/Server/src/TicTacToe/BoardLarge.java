package TicTacToe;

public class BoardLarge {
    private BoardSmall boards[][];
    private BoardSmall forcedboard;
    private Player winner;
    
    public BoardLarge() {
        this.winner = null;
        this.boards = new BoardSmall[3][3];
        this.forcedboard = null;
    }
    
    public boolean MakeMove(Player ply, int xboard, int yboard, int xpos, int ypos) {
        boolean wasvalid = true;
        if (xboard > 2 || yboard > 2 || xpos > 2 || ypos > 2)
            return false;
        
        // Check if the move was valid
        if (this.forcedboard == null || forcedboard == this.boards[xboard][yboard])
            wasvalid = this.boards[xboard][yboard].MakeMove(ply, xpos, ypos);
        
        // If it was, check which board to force next
        if (wasvalid) {
            this.forcedboard = this.boards[xpos][ypos];
            this.boards[xboard][yboard].CheckWinner();
            if (this.boards[xboard][yboard].GetWinner() != null)
                this.forcedboard = null;
        }
        return wasvalid;
    }
    
    public void CheckWinner() {
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
    
    public Player GetWinner() {
        return this.winner;
    }
    
    public BoardSmall GetBoard(int xpos, int ypos) {
        return this.boards[xpos][ypos];
    }
    
    public int GetForcedBoardNumber() {
        int boardnum = 1;
        if (this.forcedboard == null)
            return 0;
        for (int y=0; y<3; y++) {
            for (int x=0; x<3; x++) {
                if (this.forcedboard == this.boards[x][y])
                    break;
                boardnum++;
            }
        }
        return boardnum;
    }
}
