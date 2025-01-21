package NetLib;

public class BadPacketVersionException extends Exception {
    private static final long serialVersionUID = 1L;

    /**
     * An exception to throw when the client times out
     */
    public BadPacketVersionException(int version) {
        super("Bad packet version.");
    }
}