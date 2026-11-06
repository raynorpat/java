import java.rmi.*;
import java.rmi.server.*;
public class Net115RemoteImpl extends UnicastRemoteObject implements Net115Remote {
    public Net115RemoteImpl() throws RemoteException { super(); }
    public String echo(String value) throws RemoteException { return "reply:" + value; }
}
