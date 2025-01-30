package Realtime;

import java.awt.Color;

public class MovingObject {
    private static final int RECTSIZE = 16;
    
    private Vector2D pos;
    private Vector2D dir;
    private Vector2D size;
    private int speed;
    private Color col;
    
    public MovingObject(Vector2D pos) {
        this.pos = pos;
        this.dir = new Vector2D((float)Math.random(), (float)Math.random());
        this.col = Color.red;
        this.speed = 5;
        this.size = new Vector2D(RECTSIZE, RECTSIZE);
    }
    
    public Vector2D GetPos() {
        return this.pos;
    }
    
    public Vector2D GetDirection() {
        return this.dir;
    }
    
    public Vector2D GetSize() {
        return this.size;
    }
    
    public int GetSpeed() {
        return this.speed;
    }
    
    public Color GetColor() {
        return this.col;
    }
}