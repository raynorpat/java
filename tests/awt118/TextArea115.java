import java.awt.*;
import java.awt.event.*;

public class TextArea115 {
    static volatile int changes;
    static native int scrollbars(TextArea area);
    static void text(TextArea area,String expected) throws Exception {
        long end=System.currentTimeMillis()+2000;
        while (!area.getText().equals(expected) && System.currentTimeMillis()<end) Thread.sleep(10);
        if (!area.getText().equals(expected)) throw new RuntimeException("area text "+area.getText());
    }
    public static void main(String[] args) throws Exception {
        System.getProperties().put("awt.toolkit","sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        for (int mode=0;mode<4;mode++) {
            Frame frame=new Frame("Text area");
            TextArea area=new TextArea("one\ntwo\nthree",3,12,mode);
            area.select(4,7);
            area.addTextListener(new TextListener() {
                public void textValueChanged(TextEvent e) { changes++; }
            });
            frame.setLayout(null); area.setBounds(10,10,230,130); frame.add(area);
            frame.setSize(260,165); frame.show(); Thread.sleep(500);
            if (scrollbars(area)!=(mode==0 ? 3 : mode==1 ? 2 : mode==2 ? 1 : 0))
                throw new RuntimeException("scrollbar policy "+mode+"/"+scrollbars(area));
            if (area.getSelectionStart()!=4 || area.getSelectionEnd()!=7) throw new RuntimeException("area selection");
            area.replaceRange("second",4,7); text(area,"one\nsecond\nthree");
            area.insert("zero\n",0); area.append("\nfour"); text(area,"zero\none\nsecond\nthree\nfour");
            area.setText("hello"); Window115.click(area); Thread.sleep(600);
            area.select(0,0); area.setCaretPosition(5); area.requestFocus();
            if (area.getCaretPosition()!=5) throw new RuntimeException("area caret");
            int before=changes; Input115.key('x',0); text(area,"hellox");
            long end=System.currentTimeMillis()+2000;
            while (changes==before && System.currentTimeMillis()<end) Thread.sleep(10);
            if (changes==before) throw new RuntimeException("area callback");
            Input115.key(0xff0d,0); text(area,"hellox\n");
            area.setEditable(false); Input115.key('y',0); Thread.sleep(600); text(area,"hellox\n");
            Dimension size=area.getSize(); area.setFont(new Font("Dialog",Font.BOLD,18));
            if (!size.equals(area.getSize()) || area.getMinimumSize().width<=0 || area.getMinimumSize().height<=0)
                throw new RuntimeException("area font geometry");
            area.setCursor(new Cursor(Cursor.CROSSHAIR_CURSOR)); area.hide(); area.show();
            area.setText("caf\u00e9\n\u00e0 bient\u00f4t"); text(area,"caf\u00e9\n\u00e0 bient\u00f4t");
            frame.dispose(); System.gc();
        }
        System.out.println("text area edits, selection, scrolling and disposal passed");
        System.exit(0);
    }
}
