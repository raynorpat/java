public class ClinitFail {
    public static void main(String[] args) {
        for (int i = 0; i < 2; i++) {
            try {
                System.out.println("value=" + ClinitBroken.value);
            } catch (Throwable error) {
                System.out.println("attempt=" + i + " " + error.getClass().getName());
                if (error instanceof ExceptionInInitializerError)
                    System.out.println("cause=" + ((ExceptionInInitializerError)error).getException().getClass().getName());
            }
        }
        for (int i = 0; i < 2; i++) {
            try {
                Object object = new ClinitNewBroken();
                System.out.println("new=" + i + " " + object.getClass().getName());
            } catch (Throwable error) {
                System.out.println("new=" + i + " " + error.getClass().getName());
            }
        }
    }
}
class ClinitNewBroken {
    static int value = ClinitBroken.fail();
}
class ClinitBroken {
    static int value = fail();
    static int fail() { throw new IllegalStateException("fixture"); }
}
