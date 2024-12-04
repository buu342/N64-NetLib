package NetLib;

public class ClientTimeoutException extends Exception {
    private static final long serialVersionUID = 1L;

    public ClientTimeoutException(String clientaddr) {
        super("Client "+clientaddr+" timed out.");
    }
}