package NetLib;

public class ClientDisconnectException extends Exception {
    private static final long serialVersionUID = 1L;

    public ClientDisconnectException(String clientaddr) {
        super("Client "+clientaddr+" disconnected.");
    }
}