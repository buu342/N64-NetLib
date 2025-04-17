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

    // Window variables
    private Game game;
    private JFrame frame;
    private static final long serialVersionUID = 1L;

    /**
     * Preview window of the game simulation
     * @param game  The game simulation to use
     */
    PreviewWindow(Game game) {
        super();
        this.setPreferredSize(new Dimension(320, 240));
        this.game = game;
        this.frame = new JFrame();
        this.frame.setTitle("Realtime Server Preview");
        this.frame.setResizable(false);
        this.frame.add(this);
        this.frame.pack();
        this.frame.setLocationRelativeTo(null);
        this.frame.setVisible(true);
        
        // Kill the game if the preview window is closed
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

    /**
     * Draw the window every frame
     * @param g  The graphics context to draw to
     */
    public void paintComponent(Graphics g) {
        Graphics2D g2d = (Graphics2D)g;
        
        // Clear the frame
        g2d.setColor(Color.gray);
        g2d.fillRect(0, 0, this.getWidth(), this.getHeight());
        
        // Draw server objects
        for (GameObject obj : this.game.GetObjects()) {
            if (obj != null) {
                Vector2D size = obj.GetSize();
                g2d.setColor(obj.GetColor());
                g2d.fillRect((int)(obj.GetPos().GetX() - size.GetX()/2), (int)(obj.GetPos().GetY() - size.GetY()/2), (int)size.GetX(), (int)size.GetY());
            }
        }
        
        // Draw player objects
        for (Player ply : this.game.GetPlayers()) {
            if (ply != null) {
                GameObject obj = ply.GetObject();
                Vector2D size = obj.GetSize();
                g2d.setColor(obj.GetColor());
                g2d.fillRect((int)(obj.GetPos().GetX() - size.GetX()/2), (int)(obj.GetPos().GetY() - size.GetY()/2), (int)size.GetX(), (int)size.GetY());
            }
        }
    }
}