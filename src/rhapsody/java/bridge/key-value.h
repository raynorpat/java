#ifndef JDK115_BRIDGE_KEY_VALUE_H
#define JDK115_BRIDGE_KEY_VALUE_H
#import <Foundation/Foundation.h>
#include "oobj.h"
int NSHasKey(HObject *, const char *, BOOL, struct fieldblock **, BOOL *);
BOOL NSJavaValueForKey(HObject *object, NSString *key, id *value);
BOOL NSJavaTakeValueForKey(HObject *object, id value, NSString *key);
#endif
