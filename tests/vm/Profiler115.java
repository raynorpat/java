import java.io.*;
public class Profiler115 {
    static int measured(int value) { return value + 1; }
    static boolean reportContains(String name) throws IOException {
        BufferedReader input = new BufferedReader(new FileReader("java.prof"));
        boolean found = false;
        String line;
        while ((line = input.readLine()) != null) if (line.indexOf(name) >= 0) found = true;
        input.close(); return found;
    }
    public static void main(String[] args) throws Exception {
        sun.misc.VM.resumeJavaMonitor();
        int value = measured(114);
        sun.misc.VM.suspendJavaMonitor();
        sun.misc.VM.writeJavaMonitorReport();
        System.out.println("value=" + value + " recorded=" + reportContains("sun/misc/VM.suspendJavaMonitor"));
        sun.misc.VM.resetJavaMonitor();
        sun.misc.VM.writeJavaMonitorReport();
        System.out.println("reset=" + !reportContains("sun/misc/VM.suspendJavaMonitor"));
        new File("java.prof").delete();
        System.exit(0);
    }
}
