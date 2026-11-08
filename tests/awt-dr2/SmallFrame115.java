import java.awt.Frame;

public class SmallFrame115 {
    public static void main(String[] args) {
        try {
            int[][] sizes = {{0, 0}, {10, 10}, {200, 10}, {10, 100}, {200, 100}};
            for (int resize = 0; resize < 2; resize++) {
                for (int i = 0; i < sizes.length; i++) {
                    Frame frame = new Frame("small");
                    frame.setResizable(resize != 0);
                    frame.setBounds(30, 40, sizes[i][0], sizes[i][1]);
                    frame.addNotify();
                    System.out.println(resize + ":" + sizes[i][0] + "x" + sizes[i][1] + ":" +
                        frame.getBounds() + ":" + frame.getPeer().getLocationOnScreen() + ":" + frame.getInsets());
                    frame.dispose();
                }
            }
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
