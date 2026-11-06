import java.rmi.*;
public interface Net115Remote extends Remote {
    String echo(String value) throws RemoteException;
}
