import java.awt.*;

public class Label115 {
    public static void main(String[] args) {
        try {
            Frame f = new Frame("Label115"); f.setLayout(null); f.setBounds(80,90,320,180);
            Label label = new Label("Label text",Label.LEFT);
            label.setBackground(Color.white); label.setForeground(Color.black);
            label.setFont(new Font("Dialog",Font.PLAIN,14));
            label.setBounds(20,50,260,60); f.add(label); f.show(); Thread.sleep(150);
            int left = WinAwtProbe.inkLeft("Label115","Label text");
            label.setAlignment(Label.CENTER);
            int center = WinAwtProbe.inkLeft("Label115","Label text");
            label.setAlignment(Label.RIGHT);
            int right = WinAwtProbe.inkLeft("Label115","Label text");
            if (left < 0 || center <= left || right <= center) throw new RuntimeException("alignment "+left+","+center+","+right);
            label.setText("Renamed \u00e9");
            if (WinAwtProbe.inkLeft("Label115","Renamed \u00e9") < 0) throw new RuntimeException("changed label");
            Dimension size = label.getPreferredSize();
            if (size.width <= 0 || size.height <= 0) throw new RuntimeException("label size");
            label.disable(); label.enable(); label.setText(""); f.dispose(); System.gc();
            System.out.println("label text, native alignment, metrics and retirement passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
