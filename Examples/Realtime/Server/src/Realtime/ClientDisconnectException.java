package Realtime;

public class ClientDisconnectException extends Exception {
    private static final long serialVersionUID = 1L;

    /**
     * An exception to throw when the client disconnects willingly
     */
    public ClientDisconnectException(String clientaddr) {
        super("Client "+clientaddr+" disconnected.");
    }
}