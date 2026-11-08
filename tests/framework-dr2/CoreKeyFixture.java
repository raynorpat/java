
public class CoreKeyFixture extends KVCFixture {
 public static String probe() {
  CoreKeyFixture o=new CoreKeyFixture(); String r="";
  r+=CoreKeyApi.hasKey(o,"i")+","+CoreKeyApi.hasKey(o,"missing")+","+CoreKeyApi.hasKey(o,"array")+";";
  r+=CoreKeyApi.getIntValue(o,"i")+","+CoreKeyApi.getIntValue(o,"precedence")+","+CoreKeyApi.getIntValue(o,"hidden")+";";
  r+=CoreKeyApi.getLongValue(o,"j")+","+CoreKeyApi.getFloatValue(o,"d")+","+CoreKeyApi.getValue(o,"text")+";";
  CoreKeyApi.setIntValue(o,"method",34); CoreKeyApi.setLongValue(o,"longMethod",8589934595L);CoreKeyApi.setFloatValue(o,"doubleMethod",2.25f);CoreKeyApi.setValue(o,"stringMethod","changed");
  r+=o.i+","+o.j+","+o.d+","+o.text+";";
  r+=CoreKeyApi.getLongValue(o,"longMethod")+","+CoreKeyApi.getFloatValue(o,"floatMethod")+","+CoreKeyApi.getFloatValue(o,"doubleMethod")+","+CoreKeyApi.getIntValue(o,"charMethod")+";";
  Object marker=new Integer(42);CoreKeyApi.setValue(o,"object",marker);r+=(CoreKeyApi.getValue(o,"object")==marker)+";";
  try {CoreKeyApi.getIntValue(o,"text");}catch(Throwable e){r+=e+";";}
  try {CoreKeyApi.setFloatValue(o,"text",1);}catch(Throwable e){r+=e+";";}
  try {CoreKeyApi.setValue(o,"text",marker);}catch(Throwable e){r+=e+";";}
  try {CoreKeyApi.hasKey(null,"i");}catch(Throwable e){r+=e+";";}
  try {CoreKeyApi.setIntValue(null,"i",1);}catch(Throwable e){r+=e+";";}
  return r;
 }
}
