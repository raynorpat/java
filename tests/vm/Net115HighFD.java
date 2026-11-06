import java.net.*;
import java.io.*;
public class Net115HighFD {
    public static void main(String[] args) throws Exception {
        DatagramSocket[] held = new DatagramSocket[1100];
        try {
            InetAddress loop = InetAddress.getByName("127.0.0.1");
            for (int i = 0; i < held.length; i++) held[i] = new DatagramSocket(0, loop);
            DatagramSocket last = held[held.length - 1];
            last.setSoTimeout(30);
            try {
                last.receive(new DatagramPacket(new byte[1], 1));
                throw new RuntimeException("high descriptor did not time out");
            } catch (InterruptedIOException expected) {}
            System.out.println("high-fd=ok");
        } finally {
            for (int i = 0; i < held.length; i++) if (held[i] != null) held[i].close();
        }
    }
}
