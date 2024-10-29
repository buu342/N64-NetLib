package TicTacToe;

public class Game implements Runnable  {
    enum GameState {
        WAITING,
        PLAYING,
        ENDED,
    }
    
    private GameState state = GameState.WAITING;
    private Player players[];
    private BoardLarge board;
    
    public Game() {
        this.players = new Player[2];
        System.out.println("Tic Tac Toe created");
    }
    
    public void run() {
        try {
            Player turn = null;
            while (true) {
                this.state = GameState.WAITING;
                
                // Wait for the room to be full
                System.out.println("Waiting for players");
                while (this.PlayerCount() < 2)
                    Thread.sleep(1000);
                
                // If no previous player turn was chosen, then give the first player the first turn
                if (turn == null)
                    turn = this.players[0];
                
                // Initialize the game
                board = new BoardLarge();
                
                // Game loop
                System.out.println("New game started");
                this.state = GameState.PLAYING;
                while (this.state != GameState.ENDED)
                {
                    // Check for winners
                    board.CheckWinner();
                    if (board.GetWinner() != null)
                        this.state = GameState.ENDED;
                    
                    // Change player turn
                    if (turn == this.players[0])
                        turn = this.players[1];
                    else
                        turn = this.players[0];
                    System.out.println("Player " + turn.GetNumber() + "'s turn");
                }
                
                // Game ended. Show the winner for 5 seconds
                System.out.println("Game Ended.");
                if (board.GetWinner() != null)
                    System.out.println("Player " + board.GetWinner().GetNumber() + " wins!");
                else
                    System.out.println("Tie game!");
                this.state = GameState.ENDED;
                Thread.sleep(5000);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    
    public Player ConnectPlayer() {
        boolean foundslot = false;
        Player ply = new Player();
        for (int i=0; i<this.players.length; i++) {
            if (this.players[i] == null) {
                this.players[i] = ply;
                ply.SetNumber(i);
                foundslot = true;
                break;
            }
        }
        if (foundslot)
            return ply;
        return null;
    }
    
    public int PlayerCount() {
        int count = 0;
        for (int i=0; i<this.players.length; i++)
            if (this.players[i] != null)
                count++;
        return count;
    }
    
    public void DisconnectPlayer(Player ply) {
        for (int i=0; i<this.players.length; i++) {
            if (this.players[i] == ply) {
                this.players[i] = null;
                return;
            }
        }
    }
}