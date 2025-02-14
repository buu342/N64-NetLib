package Realtime;

public class Vector2D {
    private float x;
    private float y;
    private float prevx;
    private float prevy;
    
    public Vector2D(float x, float y) {
        this.x = x;
        this.y = y;
        this.prevx = x;
        this.prevy = y;
    }
    
    public static Vector2D Add(Vector2D a, Vector2D b) {
        return new Vector2D(a.x + b.x, a.y + b.y);
    }
    
    public float GetX() {
        return this.x;
    }
    
    public float GetY() {
        return this.y;
    }
    
    public float GetPreviousX() {
        return this.prevx;
    }
    
    public float GetPreviousY() {
        return this.prevy;
    }
    
    public void SetX(float x) {
        this.prevx = this.x;
        this.x = x;
    }
    
    public void SetY(float y) {
        this.prevy = this.y;
        this.y = y;
    }
    
    public void Normalize() {
        float mag = ((float)Math.sqrt(this.x*this.x + this.y*this.y));
        if (mag == 0) {
            this.x = 0;
            this.y = 0;
        } else {
            mag = 1.0f/mag;
            this.x = this.x*mag;
            this.y = this.y*mag;
        }
    }
}