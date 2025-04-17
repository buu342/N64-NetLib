package NetLib;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.util.TimerTask;

class LaggedPacketTask extends TimerTask {
    
    // Sockets for communication
    DatagramSocket socket;
    DatagramPacket out;
    
    /**
     * A task for sending a packet in a timer. This is used for debugging purposes with the UDPHandler
     * @param socket  The socket to use in the connection
     * @param out     The packet to send out
     */
    LaggedPacketTask(DatagramSocket socket, DatagramPacket out)
    {
        this.socket = socket;
        this.out = out;
    }
    
    /**
     * The task performed by the timer
     */
    public void run() {
        try {
            this.socket.send(this.out);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}