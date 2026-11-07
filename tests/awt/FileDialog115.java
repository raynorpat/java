import java.awt.*;
import java.io.*;
public class FileDialog115 {
    static volatile Throwable failure;
    public static void main(String[] args) {
        try {
            Frame owner=new Frame("FileDialog115 owner"); owner.setBounds(50,60,300,220); owner.show();
            WinAwtProbe.windowState("FileDialog115 owner",5);
            String directory=new File(".").getCanonicalPath();
            for (int kind=0;kind<3;kind++) {
                final FileDialog dialog=new FileDialog(owner,"FileDialog115",kind==2 ? FileDialog.SAVE:FileDialog.LOAD);
                dialog.setDirectory(directory); dialog.setFile("initial.txt"); failure=null;
                Thread worker=new Thread(new Runnable() {
                    public void run() { try { dialog.show(); } catch (Throwable problem) { failure=problem; } }
                });
                worker.start();
                for (int j=0;!WinAwtProbe.fileDialogReady("FileDialog115") && failure==null && j<1000;j++) Thread.sleep(10);
                if (failure!=null) throw new RuntimeException("show failed: "+failure);
                if (!WinAwtProbe.fileDialogReady("FileDialog115")) throw new RuntimeException("native file dialog missing "+kind);
                String path=kind==1 ? new File(System.getProperty("java.home"),"lib\\classes.zip").getCanonicalPath()
                    : new File(directory,"awt-save-"+System.currentTimeMillis()+".txt").getAbsolutePath();
                if (kind==0) WinAwtProbe.close("FileDialog115"); else WinAwtProbe.chooseFile("FileDialog115",path);
                worker.join(4000);
                if (worker.isAlive() || failure!=null) throw new RuntimeException("file dialog return "+kind+": "+failure);
                if (kind==0 && dialog.getFile()!=null) throw new RuntimeException("cancel file");
                if (kind!=0 && !new File(dialog.getDirectory(),dialog.getFile()).getCanonicalPath().equals(path)) throw new RuntimeException("selection "+dialog.getDirectory()+dialog.getFile()+" expected "+path);
                if (!new File(".").getCanonicalPath().equals(directory)) throw new RuntimeException("current directory changed");
                dialog.dispose();
            }
            owner.dispose();
            System.out.println("file dialog cancel, open/save selection and directory restoration passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
