package TicTacToe;

public class Player {
    private String name;
    private int number;
    
    public Player() {
        
    }
    
    public void SetNumber(int number) {
        this.number = number;
    }
    
    public void SetName(String name) {
        this.name = name;
    }
    
    public int GetNumber() {
        return this.number;
    }
    
    public String GetName() {
        return this.name;
    }
}