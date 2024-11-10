package TicTacToe;

public class BoardSmall {
    private int movecount;
    private Player winner;
    private Player moves[][];
    
    public BoardSmall() {
        this.winner = null;
        this.moves = new Player[3][3];
        this.movecount = 0;
    }
    
    public boolean MakeMove(Player ply, int xpos, int ypos) {
        if (this.moves[xpos][ypos] != null)
            return false;
        this.moves[xpos][ypos] = ply;
        this.movecount++;
        this.CheckWinner();
        return true;
    }
    
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
    
    public boolean BoardFinished() {
        if (this.winner != null)
            return true;
        return (this.movecount == 9);
    }
    
    public Player GetWinner() {
        return this.winner;
    }
    
    public Player GetMove(int x, int y) {
        return this.moves[x][y];
    }
    
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
