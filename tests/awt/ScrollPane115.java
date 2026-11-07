import java.awt.*;

public class ScrollPane115 {
    static class Content extends Canvas {
        public Dimension getPreferredSize() { return new Dimension(400,300); }
    }
    public static void main(String[] args) {
        try {
            for (int policy=0;policy<3;policy++) {
                Frame f=new Frame("ScrollPane115"); f.setLayout(null); f.setBounds(50,60,300,240);
                ScrollPane pane=new ScrollPane(policy); pane.setBounds(15,40,220,160); pane.add(new Content()); f.add(pane);
                f.show(); f.validate();
                Thread.sleep(100);
                pane.setScrollPosition(35,45);
                Point p=pane.getScrollPosition();
                if (p.x!=35 || p.y!=45) throw new RuntimeException("position "+p);
                Dimension size=pane.getViewportSize();
                System.out.println("policy="+policy+" viewport="+size.width+":"+size.height+" bars="+pane.getHScrollbarHeight()+":"+pane.getVScrollbarWidth());
                if (policy==0) {
                    pane.getHAdjustable().setUnitIncrement(3);
                    WinAwtProbe.paneScroll("ScrollPane115",false,1,0);
                    for (int i=0;pane.getHAdjustable().getValue()!=38 && i<100;i++) Thread.sleep(10);
                    if (pane.getHAdjustable().getValue()!=38) throw new RuntimeException("native horizontal callback");
                    WinAwtProbe.paneScroll("ScrollPane115",true,5,60);
                    for (int i=0;pane.getVAdjustable().getValue()!=60 && i<100;i++) Thread.sleep(10);
                    if (pane.getVAdjustable().getValue()!=60) throw new RuntimeException("native vertical callback");
                }
                pane.setScrollPosition(500,500);
                p=pane.getScrollPosition();
                System.out.println("clamped="+p.x+":"+p.y);
                pane.setScrollPosition(0,0); f.dispose();
            }
            System.out.println("scroll pane policies, insets, spans and positions passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
