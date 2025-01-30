package Realtime;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Graphics2D;
import javax.swing.JFrame;

public class PreviewFrame extends JFrame {

    private Game game;
    private static final long serialVersionUID = 1L;

    PreviewFrame(Game game) {
        this.setSize(320, 240);
        this.setTitle("Realtime Server Preview");
        this.setLocationRelativeTo(null);
        this.setVisible(true);
        this.game = game;
    }
    
    public void paint(Graphics g) {
        Graphics2D g2d = (Graphics2D)g;
        
        g2d.setColor(Color.red);
        for (MovingObject obj : this.game.GetObjects()) {
            if (obj != null) {
                Vector2D size = obj.GetSize();
                g2d.fillRect((int)(obj.GetPos().x - size.x/2), (int)(obj.GetPos().y - size.y/2), (int)size.x, (int)size.y);
            }
        }
    }
}