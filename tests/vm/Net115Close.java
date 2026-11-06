import java.net.*;
import java.io.*;
public class Net115Close extends Thread {
    ServerSocket server;
    Socket socket;
    DatagramSocket datagram;
    volatile boolean started;
    volatile boolean caught;
    public void run() {
        started = true;
        try {
            if (server != null) server.accept();
            else if (socket != null) socket.getInputStream().read();
            else datagram.receive(new DatagramPacket(new byte[8], 8));
        } catch (IOException expected) { caught = true; }
    }
    void finish() throws Exception {
        setDaemon(true); start();
        while (!started) Thread.sleep(5);
        Thread.sleep(150);
        if (server != null) server.close();
        else if (socket != null) socket.close();
        else datagram.close();
        join(3000);
        if (isAlive() || !caught) throw new RuntimeException("close did not interrupt IO");
    }
    public static void main(String[] args) throws Exception {
        InetAddress loop = InetAddress.getByName("127.0.0.1");
        Net115Close accept = new Net115Close();
        accept.server = new ServerSocket(0, 5, loop); accept.finish();
        ServerSocket server = new ServerSocket(0, 5, loop);
        Socket client = new Socket(loop, server.getLocalPort());
        Net115Close read = new Net115Close();
        read.socket = server.accept(); read.finish(); client.close(); server.close();
        Net115Close receive = new Net115Close();
        receive.datagram = new DatagramSocket(0, loop); receive.finish();
        System.out.println("close=ok");
    }
}
