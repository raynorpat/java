import java.net.*;
import java.io.*;
public class Net115PeekClose extends Thread {
    DatagramSocket socket;
    volatile boolean started, caught;
    public void run() {
        started = true;
        try { socket.receive(new DatagramPacket(new byte[8], 8)); }
        catch (IOException expected) { caught = true; }
    }
    public static void main(String[] args) throws Exception {
        Net115PeekClose reader = new Net115PeekClose();
        reader.socket = new DatagramSocket(0, InetAddress.getByName("127.0.0.1"));
        reader.setDaemon(true);
        Net115PeekSecurity security = new Net115PeekSecurity();
        System.setSecurityManager(security);
        reader.start();
        while (!reader.started) Thread.sleep(5);
        Thread.sleep(150);
        reader.socket.close();
        reader.join(3000);
        if (reader.isAlive() || !reader.caught) throw new RuntimeException("peek close did not throw");
        if (security.connections != 0) throw new RuntimeException("peek published a nonexistent sender");
        System.out.println("peek-close=ok");
    }
}
class Net115PeekSecurity extends SecurityManager {
    volatile int connections;
    public void checkConnect(String host, int port) { connections++; }
    public void checkAccess(Thread thread) {}
    public void checkAccess(ThreadGroup group) {}
    public void checkRead(String file) {}
    public void checkPackageAccess(String name) {}
    public void checkPropertyAccess(String name) {}
    public void checkExit(int status) {}
}
