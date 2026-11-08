import java.awt.Canvas;
import java.awt.Component;
import java.awt.event.MouseEvent;

public class MouseValues115 {
    static { System.loadLibrary("mousevalues115"); }
    static native MouseEvent convert(Component source, int type, int flags, boolean inside);
    public static void main(String[] args) {
        Canvas source = new Canvas();
        int[] flags = {0, 0x10000, 0x20000, 0x40000, 0x80000, 0x100000, 0x400000, 0x5f0000};
        int[] modifiers = {0, 0, 1, 2, 8, 4, 0, 15};
        int[] ids = {0, 501, 502, 501, 502, 503, 506, 506, 505, 505};
        int count = 0;
        for (int type = 1; type <= 9; type++) {
            for (int variant = 0; variant < flags.length; variant++) {
                for (int hit = 0; hit < 2; hit++) {
                    long before = System.currentTimeMillis();
                    MouseEvent event = convert(source, type, flags[variant], hit != 0);
                    long after = System.currentTimeMillis();
                    int id = type >= 8 && hit != 0 ? 504 : ids[type];
                    int mask = modifiers[variant] | (type == 3 || type == 4 || type == 7 ? 4 : 0);
                    int clicks = type == 5 || type >= 8 ? 0 : 3;
                    if (event == null || event.getSource() != source || event.getID() != id ||
                        event.getModifiers() != mask || event.getX() != -13 || event.getY() != 33 ||
                        event.getClickCount() != clicks || event.isPopupTrigger() != (type == 3) ||
                        event.getWhen() < before || event.getWhen() > after)
                        throw new Error("mouse conversion type=" + type + " flags=" + flags[variant] + ": " + event);
                    count++;
                }
            }
        }
        System.out.println("DR2 mouse conversion matches all " + count + " type/modifier/hit cases");
    }
}
