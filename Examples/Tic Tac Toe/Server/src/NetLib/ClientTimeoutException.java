package NetLib;

public class ClientTimeoutException extends Exception {
    private static final long serialVersionUID = 1L;

    /**
     * An exception to throw when the client times out
     */
    public ClientTimeoutException(String clientaddr) {
        super("Client "+clientaddr+" timed out.");
    }
}