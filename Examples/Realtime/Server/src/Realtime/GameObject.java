package Realtime;

import java.awt.Color;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;

public class GameObject {
    private static final int RECTSIZE = 16;
    
    private int id;
    private Vector2D pos;
    private Vector2D dir;
    private Vector2D size;
    private int speed;
    private int oldspeed;
    private Color col;
    
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
    
    public int GetID() {
        return this.id;
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
    
    public int GetOldSpeed() {
        return this.oldspeed;
    }
    
    public int GetSpeed() {
        return this.speed;
    }
    
    public Color GetColor() {
        return this.col;
    }
    
    public void SetPos(Vector2D pos) {
        this.pos.SetX(pos.GetX());
        this.pos.SetY(pos.GetY());
    }
    
    public void SetDirection(Vector2D dir) {
        this.dir.SetX(dir.GetX());
        this.dir.SetY(dir.GetY());
    }
    
    public void SetSpeed(int speed) {
        this.oldspeed = this.speed;
        this.speed = speed;
    }
    
    public byte[] GetData() throws IOException {
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        bytes.write(ByteBuffer.allocate(4).putInt(this.id).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.pos.GetX()).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.pos.GetY()).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.dir.GetX()).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.dir.GetY()).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.size.GetX()).array());
        bytes.write(ByteBuffer.allocate(4).putFloat(this.size.GetY()).array());
        bytes.write(ByteBuffer.allocate(4).putInt(this.speed).array());
        bytes.write(ByteBuffer.allocate(1).put((byte)this.col.getRed()).array());
        bytes.write(ByteBuffer.allocate(1).put((byte)this.col.getGreen()).array());
        bytes.write(ByteBuffer.allocate(1).put((byte)this.col.getBlue()).array());
        return bytes.toByteArray();
    }
}