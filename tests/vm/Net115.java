import java.net.*;
import java.io.*;
public class Net115 {
    static void check(boolean b, String s) { if (!b) throw new RuntimeException(s); }
    public static void main(String[] args) throws Exception {
        InetAddress loop = InetAddress.getByName("127.0.0.1");
        check(loop.getHostAddress().equals("127.0.0.1"), "numeric");
        check(InetAddress.getAllByName("localhost").length > 0, "localhost");
        ServerSocket server = new ServerSocket(0, 5, loop);
        server.setSoTimeout(80);
        try { server.accept(); throw new RuntimeException("accept timeout missing"); }
        catch (InterruptedIOException good) {}
        Socket client = new Socket(loop, server.getLocalPort());
        Socket peer = server.accept();
        client.setTcpNoDelay(true); check(client.getTcpNoDelay(), "nodelay");
        client.setSoLinger(true, 3); check(client.getSoLinger() == 3, "linger");
        client.setSoLinger(false, 0); check(client.getSoLinger() == -1, "linger off");
        check(client.getLocalPort() > 0 && peer.getPort() == client.getLocalPort(), "ports");
        check(client.getLocalAddress().equals(loop), "bound address");
        peer.setSoTimeout(80);
        try { peer.getInputStream().read(); throw new RuntimeException("read timeout missing"); }
        catch (InterruptedIOException good) {}
        byte[] bytes = new byte[] {0, 1, 127, -1, 42};
        client.getOutputStream().write(bytes, 1, 3);
        DataInputStream in = new DataInputStream(peer.getInputStream());
        byte[] got = new byte[3]; in.readFully(got);
        check(got[0] == 1 && got[1] == 127 && got[2] == -1, "tcp bytes");
        client.close(); check(in.read() == -1, "eof"); peer.close();
        int unused = server.getLocalPort(); server.close();
        try { new Socket(loop, unused); throw new RuntimeException("connect succeeded"); }
        catch (ConnectException good) {}
        System.out.println("tcp=ok");
        DatagramSocket a = new DatagramSocket(0, loop), b = new DatagramSocket(0, loop);
        b.setSoTimeout(80);
        DatagramPacket packet = new DatagramPacket(new byte[3], 3);
        try { b.receive(packet); throw new RuntimeException("udp timeout missing"); }
        catch (InterruptedIOException good) {}
        a.send(new DatagramPacket(bytes, bytes.length, loop, b.getLocalPort()));
        b.receive(packet);
        check(packet.getLength() == 3 && packet.getData()[2] == 127, "udp truncation");
        check(packet.getPort() == a.getLocalPort() && packet.getAddress().equals(loop), "udp sender");
        a.close(); b.close();
        MulticastSocket m = new MulticastSocket(0);
        m.setTTL((byte)7); check(m.getTTL() == 7, "ttl");
        m.setInterface(loop); check(m.getInterface().equals(loop), "interface"); m.close();
        System.out.println("udp=ok");
        System.out.println("net115=ok");
    }
}
