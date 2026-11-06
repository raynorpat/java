import java.io.*;

public class JpegLibrary115 {
    static { System.loadLibrary("jpeg115probe"); }
    private native void decode(InputStream in, byte[] buffer);
    int width, height, rows;
    boolean gray, progressive, cancelHeader, cancelPixels;
    boolean throwHeader, throwPixels;
    int[] pixels;
    public boolean sendHeaderInfo(int w, int h, boolean g, boolean p) {
        if (throwHeader) throw new RuntimeException("jpeg header failure");
        width=w; height=h; gray=g; progressive=p; pixels=new int[w*h];
        return !cancelHeader;
    }
    public boolean sendPixels(int[] data, int row) {
        if (throwPixels) throw new RuntimeException("jpeg pixel failure");
        rows++;
        System.arraycopy(data, 0, pixels, row*width, width);
        return !cancelPixels;
    }
    public boolean sendPixels(byte[] data, int row) {
        rows++;
        for (int i=0; i<width; i++) pixels[row*width+i]=data[i]&255;
        return !cancelPixels;
    }
    static class ShortInput extends ByteArrayInputStream {
        ShortInput(byte[] data) { super(data); }
        public synchronized int read(byte[] b, int off, int len) {
            return super.read(b, off, Math.min(3,len));
        }
    }
    static class BrokenInput extends InputStream {
        public int read() throws IOException { throw new IOException("jpeg input failure"); }
    }
    static byte[] load(String path) throws Exception {
        FileInputStream in=new FileInputStream(path);
        ByteArrayOutputStream out=new ByteArrayOutputStream();
        byte[] b=new byte[1024]; int n;
        while ((n=in.read(b))>0) out.write(b,0,n);
        in.close(); return out.toByteArray();
    }
    static void run(String name, InputStream in, boolean ch, boolean cp) {
        JpegLibrary115 decoder=new JpegLibrary115();
        decoder.cancelHeader=ch; decoder.cancelPixels=cp;
        decoder.throwHeader=name.equals("headererror");
        decoder.throwPixels=name.equals("pixelerror");
        try {
            decoder.decode(in,new byte[4096]);
            long checksum=0;
            if (decoder.pixels!=null)
                for (int i=0; i<decoder.pixels.length; i++) checksum=checksum*31+decoder.pixels[i];
            System.out.println(name+"="+decoder.width+"x"+decoder.height+","+decoder.gray+","+decoder.progressive+","+checksum+",rows="+(decoder.rows>0));
        } catch (Throwable e) { System.out.println(name+"="+e.getClass().getName()+":"+e.getMessage()); }
    }
    public static void main(String[] args) throws Exception {
        byte[] rgb=load(args[0]+"/rgb.jpg");
        byte[] gray=load(args[0]+"/gray.jpg");
        byte[] progressive=load(args[0]+"/progressive.jpg");
        byte[] marker=new byte[rgb.length+9004];
        marker[0]=(byte)255; marker[1]=(byte)216;
        marker[2]=(byte)255; marker[3]=(byte)226;
        marker[4]=(byte)(9002>>8); marker[5]=(byte)9002;
        System.arraycopy(rgb,2,marker,9006,rgb.length-2);
        run("rgb",new ByteArrayInputStream(rgb),false,false);
        run("gray",new ByteArrayInputStream(gray),false,false);
        run("progressive",new ByteArrayInputStream(progressive),false,false);
        run("short",new ShortInput(rgb),false,false);
        run("shortprogressive",new ShortInput(progressive),false,false);
        run("marker",new ByteArrayInputStream(marker),false,false);
        run("shortmarker",new ShortInput(marker),false,false);
        run("truncatedmarker",new ByteArrayInputStream(marker,0,8000),false,false);
        run("truncated",new ByteArrayInputStream(rgb,0,rgb.length-2),false,false);
        run("invalid",new ByteArrayInputStream(new byte[]{1,2,3}),false,false);
        run("empty",new ByteArrayInputStream(new byte[0]),false,false);
        run("headercancel",new ByteArrayInputStream(rgb),true,false);
        run("pixelcancel",new ByteArrayInputStream(rgb),false,true);
        run("headererror",new ByteArrayInputStream(rgb),false,false);
        run("pixelerror",new ByteArrayInputStream(rgb),false,false);
        run("inputerror",new BrokenInput(),false,false);
        run("null",null,false,false);
        try { new JpegLibrary115().decode(new ByteArrayInputStream(rgb),null); }
        catch (NullPointerException e) { System.out.println("nullbuffer=java.lang.NullPointerException"); }
    }
}
