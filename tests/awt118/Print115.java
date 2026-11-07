import java.awt.*;
import java.awt.image.*;
import java.io.*;
import java.util.Properties;
import sun.awt.motif.PSPrintJob;

public class Print115 {
    static native boolean choose(boolean cancel);
    static volatile PrintJob selected;
    static volatile Throwable failure;
    static void dialog(final Frame frame,final String path,boolean cancel) throws Exception {
        final Properties properties=new Properties();
        properties.put("awt.print.destination","file"); properties.put("awt.print.fileName",path);
        selected=null; failure=null;
        Thread worker=new Thread(new Runnable() { public void run() {
            try { selected=Toolkit.getDefaultToolkit().getPrintJob(frame,"Dialog print",properties); }
            catch(Throwable error) { failure=error; }
        }});
        worker.start();
        long end=System.currentTimeMillis()+8000;
        boolean clicked=false;
        while(worker.isAlive() && System.currentTimeMillis()<end && !(clicked=choose(cancel))) Thread.sleep(50);
        worker.join(3000);
        if(failure!=null) throw new RuntimeException(failure.toString());
        if(!clicked || worker.isAlive() || (cancel ? selected!=null : selected==null)) throw new RuntimeException("print dialog result cancel="+cancel+" clicked="+clicked+" alive="+worker.isAlive()+" job="+(selected!=null));
        if(selected!=null) { Graphics graphics=selected.getGraphics(); graphics.setFont(new Font("Dialog",Font.PLAIN,12)); graphics.drawString("Toolkit print",10,20); graphics.dispose(); selected.end(); }
    }
    public static void main(String[] args) throws Exception {
        try {
            Toolkit.getDefaultToolkit();
            System.loadLibrary("motif118xprobe");
            ByteArrayOutputStream bytes=new ByteArrayOutputStream();
            PSPrintJob job=new PSPrintJob(new PrintStream(bytes),"Motif reconstruction",new Dimension(612,792),72);
            if(job.getPageResolution()!=72 || job.lastPageFirst()) throw new RuntimeException("page configuration");
            Image image=Toolkit.getDefaultToolkit().createImage(new MemoryImageSource(2,2,new int[]{0xffff0000,0xff00ff00,0xff0000ff,0xffffffff},0,2));
            for(int page=0;page<2;page++) {
                Graphics graphics=job.getGraphics();
                graphics.setColor(Color.red); graphics.fillRect(10,10,30,20);
                graphics.setColor(Color.black);
                graphics.setFont(new Font("Dialog",Font.PLAIN,12)); graphics.drawString("Page "+page,10,60);
                graphics.drawImage(image,10,80,20,20,null); graphics.dispose();
            }
            job.end(); image.flush();
            String output=bytes.toString();
            if(output.indexOf("%!PS-Adobe")!=0 || output.indexOf("%%Page:  1")<0 || output.indexOf("%%Page:  2")<0 || output.indexOf("colorimage")<0)
                throw new RuntimeException("PostScript structure: "+output.indexOf("%!PS-Adobe")+","+output.indexOf("%%Page:  1")+","+output.indexOf("%%Page:  2")+","+output.indexOf("colorimage"));
            if(args.length>0) {
                FileOutputStream file=new FileOutputStream(args[0]); file.write(bytes.toByteArray()); file.close();
                String path=args[0]+".dialog"; new File(path).delete();
                Frame frame=new Frame(); frame.setSize(160,100); frame.show();
                dialog(frame,path,true);
                if(new File(path).exists()) throw new RuntimeException("cancel created file");
                dialog(frame,path,false); frame.dispose();
                if(new File(path).length()==0) throw new RuntimeException("missing file output");
            }
            System.out.println("PostScript pages, text and color images passed"); System.exit(0);
        } catch(Throwable error) { error.printStackTrace(); System.exit(1); }
    }
}
