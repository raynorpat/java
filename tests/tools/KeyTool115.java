/* Isolate javakey's database without changing its original Java tool class. */
public class KeyTool115 {
    public static void main(String[] args) throws Exception {
        java.security.Security.setProperty("identity.database", args[0]);
        String[] command = new String[args.length - 1];
        System.arraycopy(args, 1, command, 0, command.length);
        sun.security.provider.Main.main(command);
    }
}
