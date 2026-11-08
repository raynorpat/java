import java.awt.*;
public class Fonts115 {
    static native boolean mixedLabel(Component label);
    public static void main(String[] args) {
        try { run(); } catch(Throwable error) { error.printStackTrace(); System.exit(1); }
    }
    static void run() {
        System.getProperties().put("awt.toolkit", "sun.awt.motif.MToolkit");
        Toolkit toolkit = Toolkit.getDefaultToolkit();
        System.loadLibrary("motif118xprobe");
        String[] names = { "Dialog", "Helvetica", "TimesRoman", "Courier", "DialogInput" };
        for (int i=0; i<names.length; i++) for (int style=0; style<4; style++) {
            Font font = new Font(names[i], style, 12);
            FontMetrics metrics = toolkit.getFontMetrics(font);
            if (metrics.getAscent() <= 0 || metrics.getDescent() < 0 || metrics.getMaxAdvance() <= 0)
                throw new RuntimeException("metrics " + font);
            if (metrics.getHeight() != metrics.getAscent()+metrics.getDescent()+metrics.getLeading())
                throw new RuntimeException("height");
            byte[] bytes = {65, 66, 67};
            if (metrics.bytesWidth(bytes,0,3) != metrics.stringWidth("ABC"))
                throw new RuntimeException("width "+font+" bytes="+metrics.bytesWidth(bytes,0,3)+" string="+metrics.stringWidth("ABC"));
            if (metrics.bytesWidth(bytes,3,0) != 0) throw new RuntimeException("empty width");
            try { metrics.bytesWidth(bytes,2,2); throw new RuntimeException("bounds accepted"); }
            catch (ArrayIndexOutOfBoundsException expected) {}
            try { metrics.bytesWidth(null,0,0); throw new RuntimeException("null accepted"); }
            catch (NullPointerException expected) {}
        }
        Frame frame=new Frame("Mixed font label");
        Label label=new Label("A\u03b1B");
        Button button=new Button("A\u03b1B"); Checkbox checkbox=new Checkbox("A\u03b1B");
        List list=new List(); list.add("A\u03b1B"); Choice choice=new Choice(); choice.add("A\u03b1B");
        frame.setLayout(new GridLayout(5,1));
        frame.add(label); frame.add(button); frame.add(checkbox); frame.add(list); frame.add(choice);
        frame.setSize(240,260); frame.show();
        if (!mixedLabel(label)) throw new RuntimeException("mixed font label lost encoded segments or fonts");
        Component[] widgets={button,checkbox,list,choice};
        for (int i=0;i<widgets.length;i++)
            if (!mixedLabel(widgets[i])) throw new RuntimeException("mixed font widget "+widgets[i].getClass().getName());
        label.setFont(new Font("Serif",Font.BOLD,18));
        label.setText("C\u03b1D");
        if (!mixedLabel(label)) throw new RuntimeException("mixed font label replacement lost segments or fonts");
        choice.setFont(new Font("Serif",Font.BOLD,18)); choice.removeAll(); choice.add("C\u03b1D");
        if (!mixedLabel(choice)) throw new RuntimeException("new choice item lost font inheritance");
        frame.dispose(); System.gc();
        System.out.println("font metrics and byte bounds passed");
        System.exit(0);
    }
}
