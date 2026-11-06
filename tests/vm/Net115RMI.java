import java.rmi.*;
import java.rmi.registry.*;
import java.net.*;
public class Net115RMI {
    public static void main(String[] args) throws Exception {
        try {
            System.getProperties().put("java.rmi.server.hostname", "127.0.0.1");
            ServerSocket probe = new ServerSocket(0);
            int port = probe.getLocalPort(); probe.close();
            Registry local = LocateRegistry.createRegistry(port);
            Net115RemoteImpl implementation = new Net115RemoteImpl();
            local.rebind("echo", implementation);
            Registry remote = LocateRegistry.getRegistry("127.0.0.1", port);
            Net115Remote stub = (Net115Remote)remote.lookup("echo");
            if (stub == implementation) throw new RuntimeException("local reference");
            if (!stub.echo("loopback").equals("reply:loopback")) throw new RuntimeException("reply");
            local.unbind("echo");
            if (remote.list().length != 0) throw new RuntimeException("registry list");
            System.out.println("rmi=ok");
            System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
