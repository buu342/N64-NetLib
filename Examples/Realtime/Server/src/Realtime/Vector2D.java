package Realtime;

public class Vector2D {
    public float x;
    public float y;
    
    public Vector2D(float x, float y) {
        this.x = x;
        this.y = y;
    }
    
    public static Vector2D Add(Vector2D a, Vector2D b) {
        return new Vector2D(a.x + b.x, a.y + b.y);
    }
    
    public void Normalize() {
        float mag = 1.0f/((float)Math.sqrt(this.x*this.x + this.y*this.y));
        this.x = this.x*mag;
        this.y = this.y*mag;
    }
}