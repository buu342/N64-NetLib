package Realtime;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import javax.swing.JFrame;
import javax.swing.JPanel;

public class PreviewWindow extends JPanel {

    private Game game;
    private JFrame frame;
    private static final long serialVersionUID = 1L;

    PreviewWindow(Game game) {
        super();
        this.setPreferredSize(new Dimension(320, 240));
        this.game = game;
        this.frame = new JFrame();
        this.frame.setTitle("Realtime Server Preview");
        this.frame.add(this);
        this.frame.pack();
        this.frame.setLocationRelativeTo(null);
        this.frame.setVisible(true);
    }
    
    public void paintComponent(Graphics g) {
        Graphics2D g2d = (Graphics2D)g;
        
        // Clear the frame
        g2d.setColor(Color.gray);
        g2d.fillRect(0, 0, this.getWidth(), this.getHeight());
        
        // Draw server objects
        g2d.setColor(Color.red);
        for (MovingObject obj : this.game.GetObjects()) {
            if (obj != null) {
                Vector2D size = obj.GetSize();
                g2d.fillRect((int)(obj.GetPos().x - size.x/2), (int)(obj.GetPos().y - size.y/2), (int)size.x, (int)size.y);
            }
        }
    }
}