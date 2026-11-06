import java.io.*;

public class ExecLifecycle {
    static void close(Process p) throws Exception {
        p.getInputStream().close();
        p.getErrorStream().close();
        p.getOutputStream().close();
    }
    public static void main(String[] args) throws Exception {
        boolean win = System.getProperty("os.name").startsWith("Windows");
        int mode = args.length == 0 ? -1 : Integer.parseInt(args[0]);
        Process p;
        if (mode == 0 || mode == -1) {
        String[] exit = win ? new String[] {"cmd", "/c", "exit 7"}
                            : new String[] {"/bin/sh", "-c", "exit 7"};
        p = Runtime.getRuntime().exec(exit);
        System.out.println("status=" + p.waitFor() + ":" + p.exitValue() + ":" + p.waitFor());
        close(p);
        }
        if (mode == 1 || mode == -1) {
        try {
            Runtime.getRuntime().exec(new String[] {"missing-process-115-not-a-command"});
            System.out.println("missing=false");
        } catch (IOException expected) { System.out.println("missing=true"); }
        }
        if (mode == 2 || mode == -1) {
        String[] sleepy = win ? new String[] {"cmd", "/c", "ping -n 30 127.0.0.1 >nul"}
                              : new String[] {"/bin/sleep", "30"};
        p = Runtime.getRuntime().exec(sleepy);
        try { p.exitValue(); System.out.println("running=false"); }
        catch (IllegalThreadStateException expected) { System.out.println("running=true"); }
        p.destroy();
        System.out.println("destroyed=" + (p.waitFor() != 0));
        close(p);
        }
        if (mode == 3 || mode == -1) {
        String[] environment = win ? new String[] {"cmd", "/c", "echo %PROCESS115%"}
                                   : new String[] {"/usr/bin/printenv", "PROCESS115"};
        p = Runtime.getRuntime().exec(environment, new String[] {"PROCESS115=env115"});
        BufferedReader input = new BufferedReader(new InputStreamReader(p.getInputStream(), "8859_1"));
        System.out.println("environment=" + input.readLine());
        System.out.println("envexit=" + p.waitFor());
        close(p);
        }
    }
}
