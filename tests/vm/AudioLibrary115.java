import sun.audio.*;
import java.io.*;
public class AudioLibrary115 {
 static void check(boolean b) { if(!b) throw new RuntimeException("audio check"); }
 public static void main(String[] args) throws Exception {
  System.loadLibrary("mmedia");
  check(AudioDevice.device.openChannels()==0);
  ByteArrayOutputStream out=new ByteArrayOutputStream(); DataOutputStream data=new DataOutputStream(out);
  data.writeInt(0x2e736e64); data.writeInt(24); data.writeInt(4); data.writeInt(1); data.writeInt(8000); data.writeInt(1);
  data.write(new byte[] {0,127,(byte)128,(byte)255});
  AudioStream audio=new AudioStream(new ByteArrayInputStream(out.toByteArray())); check(audio.getLength()==4);
  AudioDataStream decoded=new AudioDataStream(audio.getData()); check(decoded.read()==0 && decoded.read()==127 && decoded.read()==128 && decoded.read()==255 && decoded.read()==-1);
  byte[] bytes=out.toByteArray();
  try { new AudioStream(new ByteArrayInputStream(bytes,0,bytes.length-1)).getData(); throw new RuntimeException("truncated audio"); } catch(IOException ok) {}
  AudioDevice.device.close(); System.out.println("audio library passed");
 }
}
