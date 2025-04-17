package Realtime;

import java.awt.Color;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class GameObject {
    
    // Constants
    private static final int RECTSIZE = 16;
    
    // Object properties
    private int id;
    private Vector2D pos;
    private Vector2D dir;
    private Vector2D size;
    private float speed;
    private float oldspeed;
    private boolean bounce;
    private Color col;

    /**
     * Game entity representation
     * @param pos  The position of the object in the world
     */
    public GameObject(Vector2D pos) {
        this.id = Realtime.Game.NewObjectID();
        this.pos = pos;
        this.dir = new Vector2D((float)Math.random(), (float)Math.random());
        this.dir.Normalize();
        this.col = new Color((int)(Math.random()*255), (int)(Math.random()*255), (int)(Math.random()*255), 255);
        this.speed = 0;
        this.oldspeed = this.speed;
        this.size = new Vector2D(RECTSIZE, RECTSIZE);
    }

    /**
     * Gets the unique ID of the object
     * @return  The unique ID of the object
     */
    public int GetID() {
        return this.id;
    }

    /**
     * Gets the position of the object
     * @return  The 2D position vector of the object
     */
    public Vector2D GetPos() {
        return this.pos;
    }

    /**
     * Gets the direction of the object
     * @return  The 2D direction vector of the object
     */
    public Vector2D GetDirection() {
        return this.dir;
    }

    /**
     * Gets the size of the object
     * @return  The 2D vector with the size of the object
     */
    public Vector2D GetSize() {
        return this.size;
    }

    /**
     * Gets the previous speed value of the object
     * @return  The old speed of the object
     */
    public float GetOldSpeed() {
        return this.oldspeed;
    }

    /**
     * Gets the speed value of the object
     * @return  The speed of the object
     */
    public float GetSpeed() {
        return this.speed;
    }

    /**
     * Gets the color of the object
     * @return  The color of the object
     */
    public Color GetColor() {
        return this.col;
    }

    /**
     * Updates the position of the object
     * @param pos  The 2D position vector to set
     */
    public void SetPos(Vector2D pos) {
        this.pos.SetX(pos.GetX());
        this.pos.SetY(pos.GetY());
    }

    /**
     * Updates the direction of the object
     * @param dir  The 2D direction vector to set
     */
    public void SetDirection(Vector2D dir) {
        this.dir.SetX(dir.GetX());
        this.dir.SetY(dir.GetY());
    }

    /**
     * Updates the speed of the object
     * @param speed  The speed value to set
     */
    public void SetSpeed(float speed) {
        this.oldspeed = this.speed;
        this.speed = speed;
    }

    /**
     * Sets whether or not this object bounces off walls
     * @param doesbounce  Whether to enable or disable bouncing
     */
    public void SetBounce(boolean doesbounce) {
        this.bounce = doesbounce;
    }

    /**
     * Gets whether or not this object bounces off walls
     * @return  Whether the object bounces off walls
     */
    public boolean GetBounce() {
        return this.bounce;
    }

    /**
     * Gets a byte representation of the object's data
     * @return  The object's representation as a byte array
     */
    public byte[] GetData() throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        bytes.write(ByteBuffer.allocate(4).putInt(this.id).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.pos.GetX()).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.pos.GetY()).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.dir.GetX()).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.dir.GetY()).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.size.GetX()).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.size.GetY()).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.speed).array());
        bytes.write(ByteBuffer.allocate(1).put((byte)(this.bounce ? 1 : 0)).array());
        bytes.write(ByteBuffer.allocate(1).put((byte)this.col.getRed()).array());
        bytes.write(ByteBuffer.allocate(1).put((byte)this.col.getGreen()).array());
        bytes.write(ByteBuffer.allocate(1).put((byte)this.col.getBlue()).array());
        return bytes.toByteArray();
    }
}