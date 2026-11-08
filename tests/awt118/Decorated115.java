import java.awt.*;
import java.awt.event.*;

public class Decorated115 {
    static native void wm(Window window, int action);
    static volatile int moved, resized, iconified, restored;
    static void checkMove(final Window window) throws Exception {
        window.addComponentListener(new ComponentAdapter() {
            public void componentMoved(ComponentEvent e) { moved++; }
            public void componentResized(ComponentEvent e) { resized++; }
        });
        Thread.sleep(200);
        for(int pass=0;pass<2;pass++) {
            int before=moved, sizeBefore=resized;
            wm(window,pass==0 ? 0 : 3);
            Thread.sleep(500);
            if (moved==before || !window.getLocation().equals(window.getLocationOnScreen()))
                throw new RuntimeException("WM move not reported: "+window.getLocation()+" screen "+window.getLocationOnScreen()+" events "+(moved-before));
            if(resized!=sizeBefore) throw new RuntimeException("WM movement emitted resize");
            if(pass==1 && window.getLocation().x>=0) throw new RuntimeException("negative WM move not exercised");
        }
    }
    static class Preferred extends Canvas {
        public Dimension getPreferredSize() { return new Dimension(200,100); }
    }
    public static void main(String[] args) {
        try {
            Toolkit.getDefaultToolkit();
            System.loadLibrary("motif118xprobe");
            for (int cycle=0; cycle<4; cycle++) {
                Frame frame = new Frame("Decorated frame");
                Button button = new Button("Client");
                frame.add(button);
                frame.setBounds(70, 80, 320, 220);
                long beforeMap=System.currentTimeMillis();
                frame.addNotify();
                frame.setSize(321,221); frame.setSize(320,220);
                if(System.currentTimeMillis()-beforeMap>2000)
                    throw new RuntimeException("unmapped resize blocked AWT monitor");
                frame.show();
                Thread.sleep(700);
                Insets insets = frame.getInsets();
                if (insets.top <= 0 || insets.left <= 0 || insets.bottom <= 0)
                    throw new RuntimeException("missing decorations: " + insets);
                long resizeStart=System.currentTimeMillis();
                frame.setSize(350, 240);
                if(System.currentTimeMillis()-resizeStart>2000)
                    throw new RuntimeException("window manager resize blocked AWT monitor");
                Thread.sleep(400);
                frame.validate();
                Point outer = frame.getLocationOnScreen();
                Point inner = button.getLocationOnScreen();
                if (inner.x-outer.x != insets.left || inner.y-outer.y != insets.top)
                    throw new RuntimeException("client origin: " + outer + " " + inner + " " + insets);
                if (button.getSize().width != 350-insets.left-insets.right ||
                    button.getSize().height != 240-insets.top-insets.bottom)
                    throw new RuntimeException("client size: " + button.getSize());
                frame.remove(button);
                Graphics graphics=frame.getGraphics();
                graphics.setColor(Color.red);
                graphics.fillRect(insets.left, insets.top, 5, 5);
                graphics.dispose();
                Toolkit.getDefaultToolkit().sync();
                if ((Window115.pixel(frame, 1, 1)&0xffffff)!=0xff0000)
                    throw new RuntimeException("frame graphics origin");
                frame.dispose();
            }
            Frame packed=new Frame("Packed frame");
            Canvas preferred=new Preferred(); packed.add(preferred); packed.pack(); packed.show();
            Thread.sleep(500);
            if (preferred.getSize().width!=200 || preferred.getSize().height!=100)
                throw new RuntimeException("packed content clipped: "+preferred.getSize());
            Dialog dialog=new Dialog(packed,"Packed dialog",false);
            Canvas dialogCanvas=new Preferred(); dialog.add(dialogCanvas); dialog.pack(); dialog.show();
            Thread.sleep(500);
            if (dialog.getInsets().top<=0 || dialogCanvas.getSize().width!=200 || dialogCanvas.getSize().height!=100)
                throw new RuntimeException("decorated dialog packing: "+dialog.getInsets()+" "+dialogCanvas.getSize());
            dialog.dispose(); packed.dispose();
            final Frame events=new Frame("WM event callbacks");
            events.setBounds(40,50,240,160); events.show(); Thread.sleep(500);
            events.addWindowListener(new WindowAdapter() {
                public void windowIconified(WindowEvent e) { iconified++; }
                public void windowDeiconified(WindowEvent e) { restored++; if(restored==4) events.dispose(); }
            });
            String errors="";
            try { checkMove(events); } catch (Exception e) { errors+=e+"\n"; }
            Dialog movedDialog=new Dialog(events,"Moved dialog",false);
            movedDialog.setBounds(60,70,200,120); movedDialog.show(); Thread.sleep(500);
            try { checkMove(movedDialog); } catch (Exception e) { errors+=e+"\n"; }
            movedDialog.dispose();
            for(int cycle=0;cycle<3;cycle++) {
                wm(events,1); Thread.sleep(500);
                wm(events,2); Thread.sleep(500);
            }
            if(iconified!=3 || restored!=3) errors+="WM icon callbacks: "+iconified+"/"+restored+"\n";
            events.hide(); Thread.sleep(200); events.show(); Thread.sleep(300);
            if(iconified!=3 || restored!=3) errors+="hide/show reported iconification\n";
            wm(events,1); Thread.sleep(500);
            wm(events,2); Thread.sleep(500);
            if(restored!=4 || events.getPeer()!=null) errors+="restore callback disposal failed\n";
            events.dispose();
            if(errors.length()!=0) throw new RuntimeException(errors);
            System.out.println("decorated frame bounds and client layout passed");
            System.exit(0);
        } catch (Throwable failure) {
            failure.printStackTrace();
            System.exit(1);
        }
    }
}
