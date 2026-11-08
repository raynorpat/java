import java.awt.Canvas;
import java.awt.Component;
import java.awt.event.KeyEvent;

public class KeyValues115 {
    static { System.loadLibrary("keyvalues115"); }
    static native KeyEvent convert(Component source, int type, int flags, char character, char plain);
    public static void main(String[] args) {
        Canvas source = new Canvas();
        // character, ignoring-modifiers character, flags, virtual code, Java character, modifiers
        int[][] cases = {
            {'a','a',0,65,'a',0}, {'A','A',0x20000,65,'A',1},
            {1,'a',0x40000,65,1,2}, {'z','z',0x100000,90,'z',4},
            {'0','0',0,48,'0',0}, {'9','9',0x200000,105,'9',0},
            {'+', '+',0x200000,107,'+',0}, {'+', '+',0x20000,61,'+',1},
            {'+', '+',0,0,'+',0}, {'!', '!',0x20000,49,'!',1},
            {'!', '!',0,0,'!',0}, {3,3,0,10,10,0}, {13,13,0,10,10,0},
            {127,127,0,8,8,0}, {63272,63272,0,127,127,0},
            {25,25,0x20000,9,9,1}, {25,25,0,0,9,0},
            {63232,63232,0,38,63232,0}, {63247,63247,0,123,63247,0},
            {63273,63273,0,36,63273,0}, {63302,63302,0,156,63302,0},
            {0xe9,0xe9,0,0,0xe9,0}, {0x3a9,0x3a9,0,0,0x3a9,0},
            {'x','x',0x5f0000,88,'x',15}, {' ', ' ',0,32,32,0},
            {'\t','\t',0,9,9,0}, {27,27,0,27,27,0}
        };
        int count = 0;
        for (int i = 0; i < cases.length; i++) {
            int[] c = cases[i];
            for (int type = 10; type <= 11; type++) {
                long before = System.currentTimeMillis();
                KeyEvent event = convert(source, type, c[2], (char)c[0], (char)c[1]);
                long after = System.currentTimeMillis();
                if (event == null || event.getSource() != source || event.getID() != type + 391 ||
                    event.getKeyCode() != c[3] || event.getKeyChar() != c[4] || event.getModifiers() != c[5] ||
                    event.getWhen() < before || event.getWhen() > after)
                    throw new Error("key conversion case=" + i + " type=" + type + ": " + event);
                count++;
            }
        }
        int[] flags = {0x80000, 0x40000, 0x20000, 0x100000, 0x400000, 0x10000};
        int[] codes = {18, 17, 16, 157, 156, 20};
        int[] modifiers = {8, 2, 1, 4, 0, 0};
        for (int i = 0; i < flags.length; i++) {
            KeyEvent event = convert(source, 12, flags[i], 'x', 'x');
            if (event == null || event.getID() != 401 || event.getKeyCode() != codes[i] ||
                event.getKeyChar() != 0 || event.getModifiers() != modifiers[i])
                throw new Error("modifier conversion: " + event);
            count++;
        }
        if (convert(source, 12, 0, 'x', 'x') != null) throw new Error("unchanged flags produced a key event");
        count++;
        System.out.println("DR2 key conversion matches all " + count + " character/modifier cases");
    }
}
