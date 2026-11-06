import java.util.zip.*;
import java.io.*;
public class ZipLibrary115 {
 static void check(boolean b) { if (!b) throw new RuntimeException("zip check"); }
 public static void main(String[] args) throws Exception {
  byte[] input = "123456789123456789123456789".getBytes("8859_1");
  CRC32 crc = new CRC32(); crc.update(input,0,9); check(crc.getValue()==3421780262L);
  Adler32 adler = new Adler32(); adler.update(input,0,9); check(adler.getValue()==152961502L);
  for(int raw=0;raw<2;raw++) {
   Deflater d = new Deflater(6,raw!=0); d.setInput(input,0,input.length); d.finish();
   ByteArrayOutputStream out = new ByteArrayOutputStream(); byte[] tiny = new byte[3];
   while(!d.finished()) { int n=d.deflate(tiny); out.write(tiny,0,n); }
   check(d.getTotalIn()==input.length); check(d.getTotalOut()==out.size()); d.reset(); check(d.getTotalIn()==0); d.end(); d.end();
   Inflater i = new Inflater(raw!=0); byte[] packed=out.toByteArray(); i.setInput(packed);
   ByteArrayOutputStream plain=new ByteArrayOutputStream();
   while(!i.finished()) { int n=i.inflate(tiny); plain.write(tiny,0,n); check(n!=0 || i.finished()); }
   check(new String(plain.toByteArray(),"8859_1").equals(new String(input,"8859_1"))); i.reset(); check(i.getTotalOut()==0); i.end(); i.end();
  }
  Deflater d = new Deflater(); d.setDictionary(input); d.setInput(input,0,input.length); d.finish(); byte[] packed=new byte[100]; int n=d.deflate(packed); d.end();
  Inflater i=new Inflater(); i.setInput(packed,0,n); byte[] plain=new byte[100]; check(i.inflate(plain)==0 && i.needsDictionary());
  try { i.setDictionary(new byte[2]); throw new RuntimeException("dictionary"); } catch(IllegalArgumentException ok) {}
  i.setDictionary(input); check(i.inflate(plain)==input.length); check(i.finished()); i.end();
  ByteArrayOutputStream out=new ByteArrayOutputStream(); ZipOutputStream z=new ZipOutputStream(out); z.putNextEntry(new ZipEntry("payload")); z.write(input); z.close();
  ZipInputStream zin=new ZipInputStream(new ByteArrayInputStream(out.toByteArray())); check(zin.getNextEntry().getName().equals("payload")); int total=0,k; while((k=zin.read(plain))!=-1) total+=k; check(total==input.length); check(zin.getNextEntry()==null); zin.close();
  // Parameter updates, input slices, and zero-length output.
  d=new Deflater(1); d.setInput(input,1,input.length-2); d.setLevel(9); d.setStrategy(Deflater.FILTERED);
  check(d.deflate(new byte[0])==0); d.finish(); out=new ByteArrayOutputStream();
  while(!d.finished()) { k=d.deflate(packed); out.write(packed,0,k); }
  check(d.getTotalIn()==input.length-2); d.end();
  i=new Inflater(); byte[] valid=out.toByteArray(); i.setInput(valid);
  check(i.inflate(plain)==input.length-2 && i.finished()); i.end();
  i=new Inflater(); valid[valid.length-1]^=1; i.setInput(valid);
  try { i.inflate(plain); throw new RuntimeException("checksum"); } catch(DataFormatException ok) {} i.end();
  i=new Inflater(); i.setInput(valid,0,valid.length-4); i.inflate(plain); check(!i.finished() && i.needsInput()); i.end();
  crc=new CRC32(); crc.update(49); check(crc.getValue()==2212294583L);
  adler=new Adler32(); adler.update(49); check(adler.getValue()==3276850L);
  try { adler.update(input,-1,2); throw new RuntimeException("bounds"); } catch(ArrayIndexOutOfBoundsException ok) {}
  d=new Deflater(); d.end(); try { d.getTotalIn(); throw new RuntimeException("ended"); } catch(NullPointerException ok) {}
  // GZIP trailer validation and truncated archive handling.
  out=new ByteArrayOutputStream(); GZIPOutputStream gz=new GZIPOutputStream(out); gz.write(input); gz.close();
  byte[] gzip=out.toByteArray(); GZIPInputStream gin=new GZIPInputStream(new ByteArrayInputStream(gzip));
  total=0; while((k=gin.read(plain))!=-1) total+=k; check(total==input.length); gin.close();
  gzip[gzip.length-8]^=1;
  try { gin=new GZIPInputStream(new ByteArrayInputStream(gzip)); while(gin.read(plain)!=-1) {} throw new RuntimeException("gzip checksum"); } catch(IOException ok) {}
  out=new ByteArrayOutputStream(); z=new ZipOutputStream(out);
  ZipEntry stored=new ZipEntry("stored"); stored.setMethod(ZipEntry.STORED); stored.setSize(input.length);
  crc=new CRC32(); crc.update(input); stored.setCrc(crc.getValue()); z.putNextEntry(stored); z.write(input); z.close();
  byte[] archive=out.toByteArray(); zin=new ZipInputStream(new ByteArrayInputStream(archive));
  check(zin.getNextEntry().getMethod()==ZipEntry.STORED); total=0; while((k=zin.read(plain))!=-1) total+=k; check(total==input.length); zin.close();
  archive[30+"stored".length()]^=1;
  // Sun 1.1.5 does not validate CRC for STORED entries.
  zin=new ZipInputStream(new ByteArrayInputStream(archive)); zin.getNextEntry(); check(zin.read(plain)>0 && plain[0]!=(byte)input[0]); zin.close();
  archive[30+"stored".length()]^=1;
  File f=new File(args[0],"library.zip"); FileOutputStream file=new FileOutputStream(f); file.write(archive); file.close();
  ZipFile zipfile=new ZipFile(f.getPath()); check(zipfile.entries().hasMoreElements()); ZipEntry ze=zipfile.getEntry("stored"); check(ze.getSize()==input.length);
  InputStream stream=zipfile.getInputStream(ze); total=0; while((k=stream.read(plain))!=-1) total+=k; check(total==input.length); stream.close(); zipfile.close();
  file=new FileOutputStream(f); file.write(archive,0,archive.length-10); file.close();
  try { zipfile=new ZipFile(f.getPath()); throw new RuntimeException("truncated central directory"); } catch(ZipException ok) {} f.delete();
  System.out.println("zip library passed");
 }
}
