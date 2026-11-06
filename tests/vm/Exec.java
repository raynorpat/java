import java.io.*;

public class Exec {
    public static void main(String[] args) throws Exception {
        boolean windows = System.getProperty("os.name").startsWith("Windows");
        String[] command = windows ? new String[] {"cmd", "/c", "echo hi"} : new String[] {"/bin/echo", "hi"};
        Process child = Runtime.getRuntime().exec(command);
        BufferedReader out = new BufferedReader(new InputStreamReader(child.getInputStream(), "8859_1"));
        System.out.println("stdout=" + out.readLine());
        System.out.println("eof=" + (out.readLine() == null));
        System.out.println("exit=" + child.waitFor());
        System.out.println("stderr=" + child.getErrorStream().read());
        out.close(); child.getErrorStream().close(); child.getOutputStream().close();
    }
}
