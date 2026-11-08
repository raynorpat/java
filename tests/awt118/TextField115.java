import java.awt.*;
import java.awt.event.*;

public class TextField115 {
    static volatile int changes,actions;
    static native String displayed(TextField field);
    static void text(TextField field,String expected) throws Exception {
        long end=System.currentTimeMillis()+2000;
        while (!field.getText().equals(expected) && System.currentTimeMillis()<end) Thread.sleep(10);
        if (!field.getText().equals(expected)) throw new RuntimeException("text: "+field.getText()+" expected "+expected);
    }
    public static void main(String[] args) throws Exception {
        System.getProperties().put("awt.toolkit","sun.awt.motif.MToolkit");
        Toolkit.getDefaultToolkit(); System.loadLibrary("motif118xprobe");
        for (int cycle=0;cycle<4;cycle++) {
            final Frame frame=new Frame("Text field");
            TextField field=new TextField("abcdef");
            field.select(1,4);
            field.addTextListener(new TextListener() {
                public void textValueChanged(TextEvent e) { changes++; }
            });
            field.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) { actions++; }
            });
            frame.setLayout(null); field.setBounds(10,10,220,35); frame.add(field);
            frame.setSize(250,90); frame.show(); Thread.sleep(100);
            text(field,"abcdef");
            if (field.getSelectionStart()!=1 || field.getSelectionEnd()!=4) throw new RuntimeException("initial selection");
            Window115.click(field); Thread.sleep(600);
            field.select(0,0); field.setCaretPosition(6); field.requestFocus(); Thread.sleep(100);
            if (field.getCaretPosition()!=6) throw new RuntimeException("caret placement "+field.getCaretPosition());
            int before=changes;
            Input115.key('g',0); text(field,"abcdefg");
            long deadline=System.currentTimeMillis()+2000;
            while (changes==before && System.currentTimeMillis()<deadline) Thread.sleep(10);
            if (changes==before) throw new RuntimeException("text callback");
            int old=actions; Input115.key(0xff0d,0);
            deadline=System.currentTimeMillis()+2000;
            while (actions==old && System.currentTimeMillis()<deadline) Thread.sleep(10);
            if (actions!=old+1) throw new RuntimeException("action callback");
            field.setEditable(false); Input115.key('h',0); Thread.sleep(600); text(field,"abcdefg");
            field.setText("caf\u00e9"); text(field,"caf\u00e9");
            field.setText("abcdefg");
            field.setEditable(true); field.setEchoChar('*');
            if (!displayed(field).equals("*******")) throw new RuntimeException("echo display");
            text(field,"abcdefg"); field.setCaretPosition(7);
            Input115.key('h',0); text(field,"abcdefgh");
            Input115.key(0xff08,0); text(field,"abcdefg");
            field.setText("secret"); text(field,"secret");
            if (!displayed(field).equals("******")) throw new RuntimeException("echo replacement");
            field.setEchoChar('#'); text(field,"secret");
            if (!displayed(field).equals("######")) throw new RuntimeException("echo change");
            StringBuffer large=new StringBuffer();
            for (int i=0;i<1500;i++) large.append('x');
            field.setText(large.toString()); text(field,large.toString());
            if (displayed(field).length()!=1500) throw new RuntimeException("echo growth");
            frame.dispose(); System.gc();
        }
        System.out.println("text field editing, selection, echo and disposal passed");
        System.exit(0);
    }
}
