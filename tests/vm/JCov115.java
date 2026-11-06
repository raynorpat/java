public class JCov115 {
    static int value = branch(4);

    static int branch(int n) {
        if (n > 2) return n;
        return -n;
    }

    static int dense(int n) {
        switch (n) {
        case 1: return 5;
        case 2: return 6;
        default: return 7;
        }
    }

    static int sparse(int n) {
        switch (n) {
        case 10: return 1;
        case 100: return 2;
        default: return 3;
        }
    }

    static int noDefault(int n) {
        switch (n) {
        case 1: return 10;
        case 3: return 30;
        }
        return 0;
    }

    public static void main(String[] args) {
        System.out.println(value + branch(1) + branch(4) + dense(2) +
            dense(8) + sparse(10) + sparse(5) + noDefault(1) + noDefault(2));
    }
}
