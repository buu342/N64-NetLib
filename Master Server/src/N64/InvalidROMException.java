package N64;

public class InvalidROMException extends Exception {
    private static final long serialVersionUID = 1L;

    /**
     * An exception to throw when a given file is not a valid N64 ROM
     */
    public InvalidROMException() {
        super("Invalid N64 ROM given");
    }
}