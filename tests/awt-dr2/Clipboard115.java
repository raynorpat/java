import java.awt.*;
import java.awt.datatransfer.*;

public class Clipboard115 {
    static { System.loadLibrary("clipboard115"); }
    static native String readNative();
    static native void trackAndFail();
    static native int liveClipboards();
    static native void writeNative(String text);
    static int lost, supplied;
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    static String read(Clipboard clipboard) throws Exception {
        return (String)clipboard.getContents(null).getTransferData(DataFlavor.stringFlavor);
    }
    public static void main(String[] args) {
        try {
            boolean tracked = args.length != 0;
            if (tracked) {
                Toolkit.getDefaultToolkit(); trackAndFail();
                try { Toolkit.getDefaultToolkit().getSystemClipboard(); throw new Error("clipboard construction succeeded"); }
                catch (RuntimeException expected) {
                    require(expected.getMessage().indexOf("clipboard construction failure") >= 0, "constructor exception");
                }
                for (int wait = 0; wait < 100 && liveClipboards() != 0; wait++) {
                    Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
                }
                require(liveClipboards() == 0, "failed clipboard retained");
            }
            final Clipboard clipboard = Toolkit.getDefaultToolkit().getSystemClipboard();
            final String text = "Java caf\u00e9 \u03a9\nclipboard";
            Transferable source = new Transferable() {
                public DataFlavor[] getTransferDataFlavors() { return new DataFlavor[] {DataFlavor.stringFlavor}; }
                public boolean isDataFlavorSupported(DataFlavor flavor) { return flavor.equals(DataFlavor.stringFlavor); }
                public Object getTransferData(DataFlavor flavor) {
                    Toolkit.getDefaultToolkit().sync(); System.gc(); supplied++; return text;
                }
            };
            ClipboardOwner owner = new ClipboardOwner() {
                public void lostOwnership(Clipboard value, Transferable contents) {
                    require(value == clipboard, "ownership source");
                    Toolkit.getDefaultToolkit().sync(); System.gc(); lost++;
                }
            };
            clipboard.setContents(source, owner);
            require(clipboard.getContents(null) == source, "local transferable identity");
            require(text.equals(readNative()) && supplied > 0, "lazy native string delivery");
            writeNative("native caf\u00e9 \u03a9");
            for (int wait = 0; wait < 200 && lost == 0; wait++) Thread.sleep(10);
            require(lost == 1, "native ownership loss");
            require(read(clipboard).equals("native caf\u00e9 \u03a9"), "native string import");
            writeNative(null); require(read(clipboard).equals("Data not readable by Java"), "unsupported native type");
            clipboard.setContents(new StringSelection("replacement"), owner);
            clipboard.setContents(new StringSelection(""), null);
            String empty = readNative();
            // The original peer and native pasteboard both notify on replacement.
            require(lost == 3 && empty.equals(""), "replacement ownership and empty string: lost=" + lost + " value=[" + empty + "]");
            final java.io.IOException unavailable = new java.io.IOException("clipboard data unavailable");
            clipboard.setContents(new Transferable() {
                public DataFlavor[] getTransferDataFlavors() { return new DataFlavor[] {DataFlavor.stringFlavor}; }
                public boolean isDataFlavorSupported(DataFlavor flavor) { return flavor.equals(DataFlavor.stringFlavor); }
                public Object getTransferData(DataFlavor flavor) throws java.io.IOException { throw unavailable; }
            }, null);
            require(readNative().equals(unavailable.toString()), "transfer exception text");
            for (int repeat = 0; repeat < 10; repeat++) {
                String value = text + repeat;
                clipboard.setContents(new StringSelection(value), null);
                require(readNative().equals(value), "repeated native export");
                writeNative(value + " native");
                require(read(clipboard).equals(value + " native"), "repeated native import");
                System.gc();
            }
            if (tracked) require(liveClipboards() == 1, "toolkit clipboard lifetime");
            System.out.println("DR2 clipboard exchanges native strings and ownership callbacks");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
