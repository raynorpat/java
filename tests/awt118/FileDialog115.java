import java.awt.*;
import java.io.*;

public class FileDialog115 {
    static native void choose(FileDialog dialog,String path,boolean cancel);
    static native int listed(FileDialog dialog);
    static volatile Throwable failure;
    static void check(boolean value,String message) { if (!value) throw new RuntimeException(message); }
    public static void main(String[] args) throws Exception {
        try { run(); } catch (Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() throws Exception {
        System.getProperties().put("awt.toolkit","sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        Frame owner=new Frame("File owner"); owner.setSize(100,100); owner.show();
        for (int cycle=0;cycle<4;cycle++) {
            final boolean cancel=(cycle&1)!=0;
            final FileDialog dialog=new FileDialog(owner,"Select",cycle<2 ? FileDialog.LOAD : FileDialog.SAVE);
            dialog.setDirectory("/tmp"); dialog.setFile("motif-test.txt"); dialog.setFont(new Font("Dialog",Font.PLAIN,12));
            dialog.setFilenameFilter(new FilenameFilter() { public boolean accept(File directory,String name) { return name.endsWith(".txt"); } });
            Thread interaction=new Thread(new Runnable() { public void run() {
                try {
                    Thread.sleep(500);
                    check(listed(dialog)==new File("/tmp").list(dialog.getFilenameFilter()).length,"native filtered file count");
                    choose(dialog,"/tmp/motif-test.txt",cancel);
                }
                catch (Throwable error) { failure=error; dialog.dispose(); }
            } });
            interaction.start(); dialog.show(); interaction.join(2000);
            if (failure!=null) throw new RuntimeException(failure.toString());
            check(cancel ? dialog.getFile()==null : "motif-test.txt".equals(dialog.getFile()),"file selection or cancellation");
            if (!cancel) check("/tmp/".equals(dialog.getDirectory()),"selected directory");
            dialog.dispose(); System.gc();
        }
        owner.dispose(); System.out.println("file dialog selection, filtering and cancellation passed"); System.exit(0);
    }
}
