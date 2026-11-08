package sun.awt;

import java.awt.*;
import java.io.*;
import java.util.StringTokenizer;

/* AppKit peers do not populate SunToolkit.peerMap. Enumerate their native
 * windows and use the shared observer's public-component snapshot format. */
public class DR2DemoObserver115 {
    static native Window[] windows();
    static native void command(int window, String action, int x, int y);
    public static void main(String[] args) throws Exception {
        Toolkit.getDefaultToolkit();
        System.load(System.getProperty("demo.probe"));
        final String output = System.getProperty("demo.snapshot");
        Thread observer = new Thread(new Runnable() {
            public void run() {
                for (;;) {
                    try {
                        Thread.sleep(300);
                        File commands = new File(output + ".command");
                        if (commands.exists()) {
                            BufferedReader input = new BufferedReader(new FileReader(commands));
                            String line;
                            while ((line = input.readLine()) != null) {
                                StringTokenizer tokens = new StringTokenizer(line);
                                int window = Integer.parseInt(tokens.nextToken());
                                String action = tokens.nextToken();
                                int x = Integer.parseInt(tokens.nextToken()), y = Integer.parseInt(tokens.nextToken());
                                command(window, action, x, y);
                            }
                            input.close();
                            // The viewer's original SecurityManager forbids delete.
                            // Acknowledge the test mailbox using its permitted rename.
                            if (!commands.renameTo(new File(output + ".done")))
                                throw new IOException("Cannot acknowledge demo command");
                        }
                        File temporary = new File(output + ".tmp");
                        PrintStream stream = new PrintStream(new FileOutputStream(temporary));
                        Window[] targets = windows();
                        for (int i = 0; i < targets.length; i++) {
                            Window window = targets[i];
                            // AppKit can call Component.setBounds while this native
                            // lookup waits for its thread. Never hold AWT's tree lock
                            // across that handoff; public accessors take their own locks.
                            DemoObserver115.record(window, stream, "0x" + Integer.toHexString(DemoObserver115.windowId(window)));
                        }
                        stream.close(); temporary.renameTo(new File(output));
                    } catch (Exception failure) { failure.printStackTrace(); }
                }
            }
        });
        observer.setDaemon(true); observer.start();
        String[] forwarded = new String[args.length - 1];
        System.arraycopy(args, 1, forwarded, 0, forwarded.length);
        Class.forName(args[0]).getMethod("main", new Class[] {forwarded.getClass()})
            .invoke(null, new Object[] {forwarded});
    }
}
