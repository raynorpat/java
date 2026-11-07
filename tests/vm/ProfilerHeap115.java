import java.io.*;
public class ProfilerHeap115 {
    static Object instance = new ProfilerHeap115();
    static Object array = new ProfilerHeap115[4];
    public static void main(String[] args) throws Exception {
        for (int round = 0; round < 3; round++) {
            sun.misc.VM.resumeJavaMonitor();
            sun.misc.VM.suspendJavaMonitor();
            sun.misc.VM.writeJavaMonitorReport();
            BufferedReader input = new BufferedReader(new FileReader("java.prof"));
            boolean objectFound = false, arrayFound = false;
            String line;
            while ((line = input.readLine()) != null) {
                line = line.trim();
                if (line.startsWith("LProfilerHeap115; 1 ")) objectFound = true;
                if (line.startsWith("[LProfilerHeap115; 1 16")) arrayFound = true;
            }
            input.close();
            if (!objectFound || !arrayFound)
                throw new RuntimeException("heap rows missing: object=" + objectFound + " array=" + arrayFound);
            sun.misc.VM.resetJavaMonitor();
        }
        new File("java.prof").delete();
        System.out.println("heap objects=1 arrays=1 reports=3");
        System.exit(0);
    }
}
