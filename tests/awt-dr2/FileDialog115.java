import java.awt.*;
import java.io.*;

public class FileDialog115 {
    static { System.loadLibrary("filedialog115"); }
    static native void armResponse(boolean approve);
    static native void track();
    static native int liveDialogs();
    static native void failConstruction(boolean fail);
    static native boolean filter(java.awt.peer.ComponentPeer peer, String path);
    static int filterCalls, completed;
    static class DisposingFileDialog extends FileDialog {
        boolean disposeOnSelection;
        DisposingFileDialog(Frame owner) { super(owner, "Dispose on selection", FileDialog.LOAD); }
        public void setFile(String file) {
            super.setFile(file);
            if (disposeOnSelection && file != null) {
                Toolkit.getDefaultToolkit().sync(); System.gc(); dispose(); completed++;
            }
        }
    }
    static void awaitReleased() throws Exception {
        for (int wait = 0; wait < 100 && liveDialogs() != 0; wait++) {
            Thread.sleep(50); Toolkit.getDefaultToolkit().sync(); System.gc();
        }
        require(liveDialogs() == 0, "native file dialogs retained: " + liveDialogs());
    }
    static void require(boolean value, String message) { if (!value) throw new Error(message); }
    public static void main(String[] args) {
        try {
            boolean tracked = args.length != 0;
            if (tracked) { Toolkit.getDefaultToolkit(); track(); }
            final String directory = new File("out/filedialog-fixtures").getAbsolutePath();
            new File(directory).mkdirs();
            FileOutputStream file = new FileOutputStream(new File(directory, "accept.txt")); file.write(65); file.close();
            new File(directory, "folder").mkdirs(); new File(directory, "Sample.app").mkdirs();
            FilenameFilter filenameFilter = new FilenameFilter() {
                public boolean accept(File parent, String name) {
                    Toolkit.getDefaultToolkit().sync(); System.gc(); filterCalls++;
                    return name.endsWith(".txt");
                }
            };
            Frame owner = new Frame("File dialog owner"); owner.setBounds(350, 100, 300, 180); owner.show();
            if (tracked) {
                FileDialog failed = new FileDialog(owner, "Failure", FileDialog.LOAD);
                failed.setDirectory(directory); failed.setFile("accept.txt"); failConstruction(true);
                try { failed.addNotify(); throw new Error("file dialog construction succeeded"); }
                catch (RuntimeException problem) { require(problem.getMessage().indexOf("file dialog construction failure") >= 0, "constructor exception"); }
                finally { failConstruction(false); }
                require(failed.getPeer() == null, "failed file dialog peer"); awaitReleased();
            }
            for (int mode = 0; mode < 2; mode++) {
                FileDialog dialog = new FileDialog(owner, "Choose a file", mode);
                dialog.setDirectory(directory); dialog.setFile(mode == FileDialog.LOAD ? "accept.txt" : "new.txt");
                dialog.setFilenameFilter(filenameFilter); dialog.addNotify();
                require(filter(dialog.getPeer(), directory + "/accept.txt"), "accepted filename");
                require(!filter(dialog.getPeer(), directory + "/reject.bin"), "rejected filename");
                int calls = filterCalls;
                require(filter(dialog.getPeer(), directory + "/folder") && filterCalls == calls, "directory bypasses Java filter");
                boolean packageAccepted = filter(dialog.getPeer(), directory + "/Sample.app");
                require(!packageAccepted && filterCalls > calls, "file package uses Java filter: accepted=" + packageAccepted + " calls=" + calls + "/" + filterCalls);
                armResponse(true); dialog.show();
                require(dialog.getFile().equals(mode == FileDialog.LOAD ? "accept.txt" : "new.txt"), "file selection: " + dialog.getFile());
                require(new File(dialog.getDirectory()).getCanonicalPath().equals(new File(directory).getCanonicalPath()) && dialog.getDirectory().endsWith("/"), "file directory: " + dialog.getDirectory());
                require(!dialog.isVisible(), "panel did not hide Java dialog");
                dialog.setFilenameFilter(null); calls = filterCalls;
                require(filter(dialog.getPeer(), directory + "/reject.bin") && filterCalls == calls, "disabled filename filter");
                dialog.setFile("cancel.txt"); armResponse(false); dialog.show();
                require(dialog.getFile() == null && !dialog.isVisible(), "file cancellation");
                Toolkit.getDefaultToolkit().sync(); System.gc(); dialog.dispose();
            }
            DisposingFileDialog disposing = new DisposingFileDialog(owner);
            disposing.setDirectory(directory); disposing.setFile("accept.txt"); disposing.disposeOnSelection = true;
            armResponse(true); disposing.show();
            require(completed == 1 && disposing.getPeer() == null, "selection callback disposal");
            owner.dispose(); if (tracked) awaitReleased();
            System.out.println("DR2 file dialogs preserve native load/save selection and cancellation");
            System.exit(0);
        } catch (Throwable problem) { problem.printStackTrace(); System.exit(1); }
    }
}
