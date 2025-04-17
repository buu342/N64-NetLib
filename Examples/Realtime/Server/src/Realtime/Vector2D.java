package Realtime;

public class Vector2D {
    
    // Vector values
    private float x;
    private float y;
    private float prevx;
    private float prevy;

    /**
     * A 2D vector object which keeps track of its previous value
     * @param x  The initial X coordinate
     * @param y  The initial Y coordinate
     */
    public Vector2D(float x, float y) {
        this.x = x;
        this.y = y;
        this.prevx = x;
        this.prevy = y;
    }

    /**
     * Adds two vectors together. 
     * Does not keep the previous value.
     * @param a  The first vector
     * @param b  The second vector
     * @return  The combined vector
     */
    public static Vector2D Add(Vector2D a, Vector2D b) {
        return new Vector2D(a.x + b.x, a.y + b.y);
    }

    /**
     * Gets the X value of the vector
     * @return  The X value
     */
    public float GetX() {
        return this.x;
    }

    /**
     * Gets the Y value of the vector
     * @return  The Y value
     */
    public float GetY() {
        return this.y;
    }

    /**
     * Gets the old X value of the vector
     * @return  The old X value
     */
    public float GetPreviousX() {
        return this.prevx;
    }

    /**
     * Gets the old Y value of the vector
     * @return  The old Y value
     */
    public float GetPreviousY() {
        return this.prevy;
    }

    /**
     * Sets the X value of the vector
     * @param x  The new X value
     */
    public void SetX(float x) {
        this.prevx = this.x;
        this.x = x;
    }

    /**
     * Sets the Y value of the vector
     * @param y  The new Y value
     */
    public void SetY(float y) {
        this.prevy = this.y;
        this.y = y;
    }

    /**
     * Normalizes the vector so it has a length of 1. Zero length vectors don't get modified. 
     * Does not keep the previous value.
     */
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