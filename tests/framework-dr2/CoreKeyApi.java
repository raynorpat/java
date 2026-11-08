public class CoreKeyApi {
 public static native boolean hasKey(Object o,String k);
 public static native Object getValue(Object o,String k);
 public static native int getIntValue(Object o,String k);
 public static native long getLongValue(Object o,String k);
 public static native float getFloatValue(Object o,String k);
 public static native void setValue(Object o,String k,Object v);
 public static native void setIntValue(Object o,String k,int v);
 public static native void setLongValue(Object o,String k,long v);
 public static native void setFloatValue(Object o,String k,float v);
}
