public class BadVerify {
    public static void main(String[] args) {
        try {
            Class.forName("BadStack");
            System.out.println("unexpected verification success");
        } catch (VerifyError error) {
            System.out.println(error.getClass().getName() + ": " + error.getMessage());
        } catch (ClassNotFoundException error) {
            throw new RuntimeException(error.toString());
        }
    }
}
