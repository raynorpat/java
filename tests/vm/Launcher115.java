class LauncherParent115 {
    public static void main(String[] args) {
        System.out.println("inherited=" + args.length);
        for (int i = 0; i < args.length; i++)
            System.out.println(args[i]);
    }
}

public class Launcher115 extends LauncherParent115 {
}
