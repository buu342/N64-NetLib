package Realtime;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
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
        this.frame.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                // Disconnect all players before ending the game
                for (Player ply : game.GetPlayers())
                    if (ply != null)
                        game.DisconnectPlayer(ply);
                System.exit(0);
            }
        });
    }
    
    public void paintComponent(Graphics g) {
        Graphics2D g2d = (Graphics2D)g;
        
        // Clear the frame
        g2d.setColor(Color.gray);
        g2d.fillRect(0, 0, this.getWidth(), this.getHeight());
        
        // Draw server objects
        for (MovingObject obj : this.game.GetObjects()) {
            if (obj != null) {
                Vector2D size = obj.GetSize();
                g2d.setColor(obj.GetColor());
                g2d.fillRect((int)(obj.GetPos().GetX() - size.GetX()/2), (int)(obj.GetPos().GetY() - size.GetY()/2), (int)size.GetX(), (int)size.GetY());
            }
        }
        
        // Draw the players
        for (Player ply : this.game.GetPlayers()) {
            if (ply != null) {
                MovingObject obj = ply.GetObject();
                Vector2D size = obj.GetSize();
                g2d.setColor(obj.GetColor());
                g2d.fillRect((int)(obj.GetPos().GetX() - size.GetX()/2), (int)(obj.GetPos().GetY() - size.GetY()/2), (int)size.GetX(), (int)size.GetY());
            }
        }
    }
}