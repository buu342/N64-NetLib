import java.net.DatagramSocket;
import NetLib.BadPacketVersionException;
import NetLib.ClientTimeoutException;
import NetLib.NetLibPacket;
import NetLib.PacketFlag;
import NetLib.S64Packet;
import NetLib.UDPHandler;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ConcurrentLinkedQueue;

public class ClientConnectionThread extends Thread {

    // Networking
    String address;
    int port;
    DatagramSocket socket;
    UDPHandler handler;
    
    // Thread communication
    ConcurrentLinkedQueue<byte[]> msgqueue = new ConcurrentLinkedQueue<byte[]>();
    
    /**
     * Thread for handling a client's UDP communication
     * @param socket   Socket to use for communication
     * @param address  Client address
     * @param port     Client port
     */
    ClientConnectionThread(DatagramSocket socket, String address, int port) {
        this.socket = socket;
        this.address = address;
        this.port = port;
        this.handler = null;
    }
    
    /**
     * Send a message to this thread
     * The message should be the raw bytes received from the client
     * @param data  The data received from the client
     * @param size  The size of the received data
     */
    public void SendMessage(byte data[], int size) {
        byte[] copy = new byte[size];
        System.arraycopy(data, 0, copy, 0, size);
        this.msgqueue.add(copy);
    }

    /**
     * Run this thread
     */
    public void run() {
        Thread.currentThread().setName("Client " + this.address + ":" + this.port);
        this.handler = new UDPHandler(this.socket, this.address, this.port);
        
        // Handle packets in a loop
        while (true) {
            try {
                byte[] data = this.msgqueue.poll();
                
                // If we received a packet, handle it based on its type
                if (data != null) {
                    if (S64Packet.IsS64PacketHeader(data)) {
                        this.HandleS64Packets(this.handler.ReadS64Packet(data));
                        return; // We can end this connection since S64 packets are one-and-done from clients
                    } else if (NetLibPacket.IsNetLibPacketHeader(data)) {
                        this.HandleNetLibPackets(this.handler.ReadNetLibPacket(data));
                        return; // In this example, we stop as soon as we handle the client's NetLib packet
                    } else {
                        System.err.println("Received unknown data from client " + this.address + ":" + this.port);
                    }
                }
            } catch (BadPacketVersionException e) {
                System.err.println(e);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * Handle S64 packets from the client
     * @param pkt  The S64 packet to handle
     * @throws ClientTimeoutException  Shouldn't happen as we are sending unreliable flags
     * @throws IOException             If an I/O error occurs
     */
    private void HandleS64Packets(S64Packet pkt) throws IOException, ClientTimeoutException {
        if (pkt == null)
            return;
        if (pkt.GetType().equals("DISCOVER")) {
            String identifier = new String(pkt.GetData(), StandardCharsets.UTF_8);
            this.handler.SendPacket(new S64Packet("DISCOVER", HelloWorldServer.ToByteArray_Client(identifier), PacketFlag.FLAG_UNRELIABLE.GetInt()));
            System.out.println("Client " + this.address + ":" + this.port + " discovered server");
        }
    }

    /**
     * Handle NetLib packets from the client
     * @param pkt  The NetLib packet to handle
     * @throws ClientTimeoutException     If the packet is sent MAX_RESEND times without an acknowledgement
     * @throws ClientDisconnectException  If the client disconnected while the server was running
     * @throws IOException                If an I/O error occurs
     * @throws InterruptedException       If this function is interrupted during sleep
     */
    private void HandleNetLibPackets(NetLibPacket pkt) throws IOException, InterruptedException, ClientTimeoutException {
        if (pkt == null)
            return;
        if (pkt.GetType() == 0) {
            int randnum = (int)(Math.random()*Integer.MAX_VALUE);
            ByteArrayOutputStream bytes = new ByteArrayOutputStream();
            bytes.write(ByteBuffer.allocate(4).putInt(randnum).array());
            this.handler.SendPacket(new NetLibPacket(0, bytes.toByteArray()));
            System.out.println("Sent client " + this.address + ":" + this.port + " the random value '" + randnum + "'.");
        } else {
            System.err.println("Received packet of unknown type " + pkt.GetType() + " from " + this.address + ":" + this.port + ".");
        }
    }
}