import java.awt.*;
import java.awt.datatransfer.*;
public class Clipboard115 {
    static { System.loadLibrary("awtclipboard115probe"); }
    static native void isolate();
    static native void verifyIsolation();
    static native void external(String value);
    static native String raw();
    static volatile int lost;
    static ClipboardOwner owner=new ClipboardOwner() {
        public void lostOwnership(Clipboard clipboard,Transferable contents) { System.gc(); lost++; }
    };
    static void detachedOwner() {
        Clipboard clipboard=new sun.awt.windows.WClipboard();
        clipboard.setContents(new StringSelection("detached"),owner);
    }
    static String text(Clipboard clipboard) throws Exception {
        return (String)clipboard.getContents(null).getTransferData(DataFlavor.stringFlavor);
    }
    public static void main(String[] args) {
        try {
            Toolkit toolkit=Toolkit.getDefaultToolkit(); Clipboard clipboard=toolkit.getSystemClipboard();
            isolate(); verifyIsolation();
            String value="a\nb\r\nc\rd\u6f22";
            clipboard.setContents(new StringSelection(value),owner);
            if(!raw().equals("a\r\nb\r\r\nc\rd\u6f22") || !text(clipboard).equals(value)) throw new RuntimeException("clipboard Unicode/newlines");
            clipboard.setContents(new StringSelection("before\u0000after"),owner);
            if(!text(clipboard).equals("before")) throw new RuntimeException("clipboard NUL boundary");
            lost=0; external("external\r\nline");
            for(int i=0;lost==0 && i<200;i++) Thread.sleep(10);
            if(lost!=1 || !text(clipboard).equals("external\nline")) throw new RuntimeException("clipboard ownership/read "+lost);
            if(System.getProperty("awt.correctedBounds")!=null) {
                detachedOwner(); System.gc(); lost=0; external("retired");
                for(int i=0;lost==0 && i<200;i++) Thread.sleep(10);
                if(lost!=1) throw new RuntimeException("detached clipboard GC root");
            }
            System.out.println("isolated clipboard Unicode, newlines, ownership and GC passed"); System.exit(0);
        } catch(Throwable t) { t.printStackTrace(); System.exit(1); }
    }
}
