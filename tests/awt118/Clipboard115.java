import java.awt.*;
import java.awt.datatransfer.*;
import java.io.*;

public class Clipboard115 {
    static volatile int losses;
    static void check(boolean value,String message) { if (!value) throw new RuntimeException(message); }
    public static void main(String[] args) throws Exception {
        try { run(); } catch (Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() throws Exception {
        System.getProperties().put("awt.toolkit","sun.awt.motif.MToolkit");
        Clipboard clipboard=Toolkit.getDefaultToolkit().getSystemClipboard();
        String helper=System.getProperty("motif.selection.helper");
        for (int cycle=0;cycle<4;cycle++) {
            clipboard.setContents(new StringSelection("from Java"),new ClipboardOwner() {
                public void lostOwnership(Clipboard value,Transferable contents) { losses++; }
            });
            Process reader=Runtime.getRuntime().exec(new String[]{helper,"read"});
            String value=new DataInputStream(reader.getInputStream()).readLine();
            int result=reader.waitFor();
            check(result==0 && "from Java".equals(value),"external clipboard read status="+result+" value="+value);
            int before=losses;
            Process owner=Runtime.getRuntime().exec(new String[]{helper,"own"});
            try {
                check("ready".equals(new DataInputStream(owner.getInputStream()).readLine()),"external clipboard owner");
                long deadline=System.currentTimeMillis()+2000;
                while (losses==before && System.currentTimeMillis()<deadline) Thread.sleep(10);
                check(losses==before+1,"clipboard ownership loss callback");
                Transferable incoming=clipboard.getContents(null);
                check(incoming.isDataFlavorSupported(DataFlavor.stringFlavor),"external target flavors");
                check("from X11".equals(incoming.getTransferData(DataFlavor.stringFlavor)),"external clipboard transfer");
            } finally {
                PrintStream command=new PrintStream(owner.getOutputStream());
                command.println("quit"); command.flush(); command.close(); owner.waitFor();
            }
            System.gc();
        }
        System.out.println("clipboard ownership, targets and external transfer passed"); System.exit(0);
    }
}
