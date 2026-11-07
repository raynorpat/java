import java.awt.*;
import java.awt.event.*;

public class Text115 {
    static volatile int changes, actions;
    public static void main(String[] args) {
        try {
            Frame f=new Frame("Text115"); f.setLayout(null); f.setBounds(50,60,320,240);
            TextField field=new TextField("caf\u00e9"); field.setBounds(20,45,240,35); f.add(field);
            field.addTextListener(new TextListener() { public void textValueChanged(TextEvent e) { System.gc(); changes++; } });
            field.addActionListener(new ActionListener() { public void actionPerformed(ActionEvent e) { actions++; } });
            f.show();
            if (!field.getText().equals("caf\u00e9")) throw new RuntimeException("initial text");
            field.setText("abcdef"); field.select(1,4);
            if (field.getSelectionStart()!=1 || field.getSelectionEnd()!=4 || !field.getSelectedText().equals("bcd")) throw new RuntimeException("field selection");
            field.setEchoChar('*'); field.setEditable(false); field.setEditable(true); field.setEchoChar((char)0);
            Thread.sleep(100); int before=changes;
            WinAwtProbe.edit("Text115","changed \u00e9",true);
            for (int i=0;(changes==before || actions==0) && i<100;i++) Thread.sleep(10);
            if (changes==before || actions!=1 || !field.getText().equals("changed \u00e9")) throw new RuntimeException("native text/action event");
            f.dispose();
            for (int policy=0;policy<4;policy++) {
                f=new Frame("Text115"); f.setLayout(null); f.setBounds(50,60,320,240);
                TextArea area=new TextArea("ab\ncd\nef",3,12,policy); area.setBounds(20,45,240,150); f.add(area); f.show();
                if (!area.getText().equals("ab\ncd\nef")) throw new RuntimeException("newline round trip");
                area.select(3,5);
                System.out.println("selection="+area.getSelectionStart()+":"+area.getSelectionEnd());
                area.insert("X\n",3); area.replaceRange("Y\nZ",1,4);
                if (!area.getText().equals("aY\nZ\ncd\nef")) throw new RuntimeException("replacement "+area.getText());
                area.setCaretPosition(5);
                System.out.println("caret="+area.getCaretPosition());
                area.append("\nend"); area.selectAll();
                if (!area.getText().endsWith("\nend")) throw new RuntimeException("append");
                f.dispose();
            }
            System.out.println("text editing, newline mapping, selection and native events passed"); System.exit(0);
        } catch (Throwable failure) { failure.printStackTrace(); System.exit(1); }
    }
}
