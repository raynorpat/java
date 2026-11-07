import java.awt.*;
public class PrintJob115 {
    static Runnable callback;
    public static void main(String[] args) {
        try {
            Frame frame=new Frame("PrintJob115");frame.show();
            if (RemainingProbe115.exports(2)!=4) throw new RuntimeException("missing print exports");
            if (!RemainingProbe115.printing(true)) throw new RuntimeException("print interception unavailable");
            for(int i=0;i<3;i++) {
                PrintJob cancelled=Toolkit.getDefaultToolkit().getPrintJob(frame,"cancel",null);
                if (cancelled!=null) throw new RuntimeException("cancelled print job");
            }
            if (RemainingProbe115.calls(0)!=3 || RemainingProbe115.calls(1)!=0)
                throw new RuntimeException("cancellation started document");
            if (System.getProperty("awt.correctedBounds")!=null) {
                RemainingProbe115.printing(false);
                PrintJob job=Toolkit.getDefaultToolkit().getPrintJob(frame,"owned virtual DC",null);
                if(job==null || job.getPageDimension().width!=612 || job.getPageDimension().height!=792 || job.getPageResolution()!=300)
                    throw new RuntimeException("page geometry or resolution");
                Graphics g=job.getGraphics();g.setColor(Color.red);g.fillRect(0,0,15,15);
                if(RemainingProbe115.pixel(4,4)!=255) throw new RuntimeException("print graphics raster");
                TextField control=new TextField("native print");frame.add(control);frame.validate();
                sun.awt.windows.PrintAccess115.print(g,control);
                if(RemainingProbe115.calls(6)!=1) throw new RuntimeException("native control WM_PRINT");
                Graphics copy=g.create();copy.dispose();
                if(RemainingProbe115.calls(3)!=0) throw new RuntimeException("copy disposal closed print page");
                g.dispose();
                if(RemainingProbe115.calls(2)!=1) throw new RuntimeException("page start");
                // close is the explicit page boundary in the shipped WGraphics API.
                ((sun.awt.windows.WGraphics)g).close((sun.awt.windows.WPrintJob)job);
                if(RemainingProbe115.calls(3)!=1) throw new RuntimeException("page close");
                copy.dispose();g=job.getGraphics();g.setColor(Color.blue);g.fillRect(1,1,5,5);
                ((sun.awt.windows.WGraphics)g).close((sun.awt.windows.WPrintJob)job);
                ((sun.awt.windows.WGraphics)g).close((sun.awt.windows.WPrintJob)job);
                g=job.getGraphics();
                job.end();job.end();g.fillRect(1,1,2,2);g.dispose();
                if(RemainingProbe115.calls(1)!=1 || RemainingProbe115.calls(2)!=3 || RemainingProbe115.calls(3)!=3 || RemainingProbe115.calls(4)!=1 || RemainingProbe115.calls(5)!=1)
                    throw new RuntimeException("document/page/end cleanup");
                RemainingProbe115.printing(false);
                final PrintJob reentrant=Toolkit.getDefaultToolkit().getPrintJob(frame,"end during native print",null);
                g=reentrant.getGraphics();
                callback=new Runnable() {public void run(){reentrant.end();System.gc();}};
                RemainingProbe115.duringPrint(callback);
                sun.awt.windows.PrintAccess115.print(g,control);
                if(RemainingProbe115.calls(7)!=1 || RemainingProbe115.calls(5)!=1)
                    throw new RuntimeException("printer DC retired during native print callback");
                g.dispose();callback=null;
            }
            frame.dispose();System.out.println("print cancellation, page boundaries and repeated end cleanup passed");System.exit(0);
        } catch(Throwable t) {t.printStackTrace();System.exit(1);}
    }
}
