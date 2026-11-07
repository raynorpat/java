/* Reconstructed from Sun JDK 1.1.5 JdbcOdbc.dll and JdbcOdbc_g.dll.
 * Method names/signatures come from the unchanged classes.zip via javah.
 * Local names and comments are reconstructed; ODBC types replace packed stack
 * temporaries shown by the binary. Buffers belong to Java and remain referenced
 * by the original bridge classes for the duration of ODBC parameter bindings.
 * As in Sun's bridge, these references do not pin the bodies across compacting
 * GC; see tests/odbc/PROVENANCE.md for the original limitation.
 */
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <string.h>
#include "sun_jdbc_odbc_JdbcOdbc.h"

#define B(a) (unhand(a)->body)
#define H(a) ((SQLHANDLE)(a))
#define S(a) ((SQLCHAR *)makeCString(a))

long sun_jdbc_odbc_JdbcOdbc_bufferToInt(struct Hsun_jdbc_odbc_JdbcOdbc* a1, HArrayOfByte * a2)
{
    return *(long *)B(a2);
}

long sun_jdbc_odbc_JdbcOdbc_allocConnect(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    SQLHDBC v = NULL;
    B(a3)[0] = SQLAllocConnect(H(a2), &v);
    return (long)v;
}

long sun_jdbc_odbc_JdbcOdbc_allocEnv(struct Hsun_jdbc_odbc_JdbcOdbc* a1, HArrayOfByte * a2)
{
    SQLHENV v = NULL;
    B(a2)[0] = SQLAllocEnv(&v);
    return (long)v;
}

long sun_jdbc_odbc_JdbcOdbc_allocStmt(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    SQLHSTMT v = NULL;
    B(a3)[0] = SQLAllocStmt(H(a2), &v);
    return (long)v;
}

void sun_jdbc_odbc_JdbcOdbc_cancel(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    B(a3)[0] = SQLCancel(H(a2));
}

void sun_jdbc_odbc_JdbcOdbc_bindColDefault(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, HArrayOfByte * a4, HArrayOfByte * a5, HArrayOfByte * a6)
{
    B(a6)[0] = SQLBindCol(H(a2), a3, SQL_C_DEFAULT, B(a4), obj_length(a4), (SQLLEN *)B(a5));
}

void sun_jdbc_odbc_JdbcOdbc_bindInParameterAtExec(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, long a5, HArrayOfByte * a6, HArrayOfByte * a7, HArrayOfByte * a8)
{
    SQLSMALLINT type = SQL_C_CHAR;
    long *value = a6 ? (long *)B(a6) : NULL;
    SQLLEN *len = a7 ? (SQLLEN *)B(a7) : NULL;
    if (value) *value = a3;
    if (len) *len = -100 - a5;
    if (a4 == -2 || a4 == -3 || a4 == -4) type = SQL_C_BINARY;
    B(a8)[0] = SQLBindParameter(H(a2), a3, SQL_PARAM_INPUT, type, a4, a5, 0, value, 4, len);
}

void sun_jdbc_odbc_JdbcOdbc_bindInParameterBinary(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, HArrayOfByte * a5, long a6, HArrayOfByte * a7, HArrayOfByte * a8, HArrayOfByte * a9)
{
    long len = 0;
    void *value = NULL;
    SQLLEN *indicator = a8 ? (SQLLEN *)B(a8) : NULL;
    if (a7) {
        value = B(a7);
        len = obj_length(a7);
        if (len > obj_length(a5)) len = obj_length(a5);
        memcpy(value, B(a5), len);
    }
    if (indicator) *indicator = len;
    B(a9)[0] = SQLBindParameter(H(a2), a3, SQL_PARAM_INPUT, SQL_C_BINARY, a4, a6, 0, value, len, indicator);
}

void sun_jdbc_odbc_JdbcOdbc_bindInParameterDate(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, long a5, long a6, HArrayOfByte * a7, HArrayOfByte * a8)
{
    DATE_STRUCT value;
    void *buffer = a7 ? B(a7) : NULL;
    value.year = a4;
    value.month = a5;
    value.day = a6;
    if (buffer) memcpy(buffer, &value, sizeof(value));
    B(a8)[0] = SQLBindParameter(H(a2), a3, SQL_PARAM_INPUT, SQL_C_DATE, SQL_C_DATE, 10, 0, buffer, sizeof(value), NULL);
}

void sun_jdbc_odbc_JdbcOdbc_bindInParameterDouble(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, long a5, double a6, HArrayOfByte * a7, HArrayOfByte * a8)
{
    void *value = a7 ? B(a7) : NULL;
    if (value) memcpy(value, &a6, 8);
    B(a8)[0] = SQLBindParameter(H(a2), a3, SQL_PARAM_INPUT, SQL_C_DOUBLE, a4, 18, a5, value, 8, NULL);
}

void sun_jdbc_odbc_JdbcOdbc_bindInParameterFloat(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, long a5, float a6, HArrayOfByte * a7, HArrayOfByte * a8)
{
    void *value = a7 ? B(a7) : NULL;
    if (value) memcpy(value, &a6, 4);
    B(a8)[0] = SQLBindParameter(H(a2), a3, SQL_PARAM_INPUT, SQL_C_FLOAT, a4, 18, a5, value, 4, NULL);
}

void sun_jdbc_odbc_JdbcOdbc_bindInParameterInteger(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, long a5, HArrayOfByte * a6, HArrayOfByte * a7)
{
    void *value = a6 ? B(a6) : NULL;
    if (value) memcpy(value, &a5, 4);
    B(a7)[0] = SQLBindParameter(H(a2), a3, SQL_PARAM_INPUT, SQL_C_SLONG, a4, 4, 0, value, 4, NULL);
}

void sun_jdbc_odbc_JdbcOdbc_bindInParameterNull(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, HArrayOfByte * a5, HArrayOfByte * a6)
{
    SQLLEN *len = (SQLLEN *)B(a5);
    *len = SQL_NULL_DATA;
    B(a6)[0] = SQLBindParameter(H(a2), a3, SQL_PARAM_INPUT, SQL_C_DEFAULT, a4, a4 == SQL_CHAR, 0, NULL, 0, len);
}

void sun_jdbc_odbc_JdbcOdbc_bindInParameterString(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, struct Hjava_lang_String * a5, long a6, long a7, HArrayOfByte * a8, HArrayOfByte * a9)
{
    char *str = (char *)S(a5);
    long len = 0;
    void *value = NULL;
    if (a8) {
        value = B(a8);
        len = obj_length(a8);
        if ((long)strlen(str) < len) len = strlen(str);
        memcpy(value, str, len);
    }
    B(a9)[0] = SQLBindParameter(H(a2), a3, SQL_PARAM_INPUT, SQL_C_CHAR, a4, a6, a7, value, len, NULL);
}

void sun_jdbc_odbc_JdbcOdbc_bindInParameterTime(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, long a5, long a6, HArrayOfByte * a7, HArrayOfByte * a8)
{
    TIME_STRUCT value;
    void *buffer = a7 ? B(a7) : NULL;
    value.hour = a4;
    value.minute = a5;
    value.second = a6;
    if (buffer) memcpy(buffer, &value, sizeof(value));
    B(a8)[0] = SQLBindParameter(H(a2), a3, SQL_PARAM_INPUT, SQL_C_TIME, SQL_C_TIME, 8, 0, buffer, sizeof(value), NULL);
}

void sun_jdbc_odbc_JdbcOdbc_bindInParameterTimestamp(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, HArrayOfByte * a11, HArrayOfByte * a12)
{
    TIMESTAMP_STRUCT value;
    void *buffer = a11 ? B(a11) : NULL;
    value.year = a4;
    value.month = a5;
    value.day = a6;
    value.hour = a7;
    value.minute = a8;
    value.second = a9;
    value.fraction = a10;
    if (buffer) memcpy(buffer, &value, sizeof(value));
    B(a12)[0] = SQLBindParameter(H(a2), a3, SQL_PARAM_INPUT, SQL_C_TIMESTAMP, SQL_C_TIMESTAMP, 29, 9, buffer, sizeof(value), NULL);
}

void sun_jdbc_odbc_JdbcOdbc_bindOutParameterString(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, long a5, HArrayOfByte * a6, HArrayOfByte * a7, HArrayOfByte * a8)
{
    B(a8)[0] = SQLBindParameter(H(a2), a3, SQL_PARAM_OUTPUT, SQL_C_CHAR, a4, obj_length(a6) - 1, a5, B(a6), obj_length(a6), (SQLLEN *)B(a7));
}

void sun_jdbc_odbc_JdbcOdbc_browseConnect(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, HArrayOfByte * a4, HArrayOfByte * a5)
{
    SQLSMALLINT len = 0;
    B(a5)[0] = SQLBrowseConnect(H(a2), S(a3), SQL_NTS, (SQLCHAR *)B(a4), obj_length(a4), &len);
}

long sun_jdbc_odbc_JdbcOdbc_colAttributes(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, HArrayOfByte * a5)
{
    SQLSMALLINT len = 0;
    SQLLEN v = 0;
    B(a5)[0] = SQLColAttributes(H(a2), a3, a4, NULL, 0, &len, &v);
    return v;
}

void sun_jdbc_odbc_JdbcOdbc_colAttributesString(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, HArrayOfByte * a5, HArrayOfByte * a6)
{
    SQLSMALLINT len = 0;
    B(a6)[0] = SQLColAttributes(H(a2), a3, a4, B(a5), obj_length(a5), &len, NULL);
}

void sun_jdbc_odbc_JdbcOdbc_columns(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, long a4, struct Hjava_lang_String * a5, long a6, struct Hjava_lang_String * a7, long a8, struct Hjava_lang_String * a9, long a10, HArrayOfByte * a11)
{
    SQLCHAR *s0 = NULL;
    SQLCHAR *s1 = NULL;
    SQLCHAR *s2 = NULL;
    SQLCHAR *s3 = NULL;
    if (!a4) s0 = S(a3);
    if (!a6) s1 = S(a5);
    if (!a8) s2 = S(a7);
    if (!a10) s3 = S(a9);
    B(a11)[0] = SQLColumns(H(a2), s0, SQL_NTS, s1, SQL_NTS, s2, SQL_NTS, s3, SQL_NTS);
}

void sun_jdbc_odbc_JdbcOdbc_columnPrivileges(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, long a4, struct Hjava_lang_String * a5, long a6, struct Hjava_lang_String * a7, long a8, struct Hjava_lang_String * a9, long a10, HArrayOfByte * a11)
{
    SQLCHAR *s0 = NULL;
    SQLCHAR *s1 = NULL;
    SQLCHAR *s2 = NULL;
    SQLCHAR *s3 = NULL;
    if (!a4) s0 = S(a3);
    if (!a6) s1 = S(a5);
    if (!a8) s2 = S(a7);
    if (!a10) s3 = S(a9);
    B(a11)[0] = SQLColumnPrivileges(H(a2), s0, SQL_NTS, s1, SQL_NTS, s2, SQL_NTS, s3, SQL_NTS);
}

long sun_jdbc_odbc_JdbcOdbc_describeParam(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, HArrayOfByte * a5)
{
    SQLSMALLINT type = 0, scale = 0, nullable = 0;
    SQLULEN size = 0;
    B(a5)[0] = SQLDescribeParam(H(a2), a3, &type, &size, &scale, &nullable);
    switch (a4) {
    case 1: return type;
    case 2: return size;
    case 3: return scale;
    case 4: return nullable;
    }
    return 0;
}

void sun_jdbc_odbc_JdbcOdbc_disconnect(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    B(a3)[0] = SQLDisconnect(H(a2));
}

void sun_jdbc_odbc_JdbcOdbc_driverConnect(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, HArrayOfByte * a4)
{
    SQLCHAR out[256];
    SQLSMALLINT len = 0;
    B(a4)[0] = SQLDriverConnect(H(a2), NULL, S(a3), SQL_NTS, out, sizeof(out), &len, SQL_DRIVER_NOPROMPT);
}

long sun_jdbc_odbc_JdbcOdbc_error(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, HArrayOfByte * a5, HArrayOfByte * a6, HArrayOfByte * a7)
{
    SQLINTEGER nativeError = 0;
    SQLSMALLINT len = 0;
    B(a7)[0] = SQLError(H(a2), H(a3), H(a4), (SQLCHAR *)B(a5), &nativeError, (SQLCHAR *)B(a6), obj_length(a6), &len);
    return nativeError;
}

void sun_jdbc_odbc_JdbcOdbc_execDirect(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, HArrayOfByte * a4)
{
    B(a4)[0] = SQLExecDirect(H(a2), S(a3), SQL_NTS);
}

void sun_jdbc_odbc_JdbcOdbc_execute(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    B(a3)[0] = SQLExecute(H(a2));
}

void sun_jdbc_odbc_JdbcOdbc_fetch(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    B(a3)[0] = SQLFetch(H(a2));
}

void sun_jdbc_odbc_JdbcOdbc_foreignKeys(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, long a4, struct Hjava_lang_String * a5, long a6, struct Hjava_lang_String * a7, long a8, struct Hjava_lang_String * a9, long a10, struct Hjava_lang_String * a11, long a12, struct Hjava_lang_String * a13, long a14, HArrayOfByte * a15)
{
    SQLCHAR *s0 = NULL;
    SQLCHAR *s1 = NULL;
    SQLCHAR *s2 = NULL;
    SQLCHAR *s3 = NULL;
    SQLCHAR *s4 = NULL;
    SQLCHAR *s5 = NULL;
    if (!a4) s0 = S(a3);
    if (!a6) s1 = S(a5);
    if (!a8) s2 = S(a7);
    if (!a10) s3 = S(a9);
    if (!a12) s4 = S(a11);
    if (!a14) s5 = S(a13);
    B(a15)[0] = SQLForeignKeys(H(a2), s0, SQL_NTS, s1, SQL_NTS, s2, SQL_NTS, s3, SQL_NTS, s4, SQL_NTS, s5, SQL_NTS);
}

void sun_jdbc_odbc_JdbcOdbc_freeConnect(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    B(a3)[0] = SQLFreeConnect(H(a2));
}

void sun_jdbc_odbc_JdbcOdbc_freeEnv(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    B(a3)[0] = SQLFreeEnv(H(a2));
}

void sun_jdbc_odbc_JdbcOdbc_freeStmt(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, HArrayOfByte * a4)
{
    B(a4)[0] = SQLFreeStmt(H(a2), (short)a3);
}

long sun_jdbc_odbc_JdbcOdbc_getConnectOption(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, short a3, HArrayOfByte * a4)
{
    long v = 0;
    B(a4)[0] = SQLGetConnectOption(H(a2), a3, &v);
    return v;
}

void sun_jdbc_odbc_JdbcOdbc_getConnectOptionString(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, short a3, HArrayOfByte * a4, HArrayOfByte * a5)
{
    B(a5)[0] = SQLGetConnectOption(H(a2), a3, B(a4));
}

void sun_jdbc_odbc_JdbcOdbc_getCursorName(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3, HArrayOfByte * a4)
{
    SQLSMALLINT len = 0;
    B(a4)[0] = SQLGetCursorName(H(a2), (SQLCHAR *)B(a3), obj_length(a3), &len);
}

long sun_jdbc_odbc_JdbcOdbc_getStmtOption(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, short a3, HArrayOfByte * a4)
{
    long v = 0;
    B(a4)[0] = SQLGetStmtOption(H(a2), a3, &v);
    return v;
}

long sun_jdbc_odbc_JdbcOdbc_getDataBinary(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, long a4, HArrayOfByte * a5, long a6, HArrayOfByte * a7)
{
    SQLLEN len = 0;
    B(a7)[0] = SQLGetData(H(a2), a3, a4, B(a5), (unsigned short)a6, &len);
    if (len > a6 || len == SQL_NO_TOTAL) len = a6;
    B(a7)[1] = len == SQL_NULL_DATA;
    return len;
}

double sun_jdbc_odbc_JdbcOdbc_getDataDouble(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, HArrayOfByte * a4)
{
    double value = 0;
    SQLLEN len = 0;
    B(a4)[0] = SQLGetData(H(a2), a3, SQL_C_DOUBLE, &value, sizeof(value), &len);
    B(a4)[1] = len == SQL_NULL_DATA;
    return value;
}

float sun_jdbc_odbc_JdbcOdbc_getDataFloat(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, HArrayOfByte * a4)
{
    float value = 0;
    SQLLEN len = 0;
    B(a4)[0] = SQLGetData(H(a2), a3, SQL_C_FLOAT, &value, sizeof(value), &len);
    B(a4)[1] = len == SQL_NULL_DATA;
    return value;
}

long sun_jdbc_odbc_JdbcOdbc_getDataInteger(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, HArrayOfByte * a4)
{
    long value = 0;
    SQLLEN len = 0;
    B(a4)[0] = SQLGetData(H(a2), a3, SQL_C_LONG, &value, sizeof(value), &len);
    B(a4)[1] = len == SQL_NULL_DATA;
    return value;
}

long sun_jdbc_odbc_JdbcOdbc_getDataString(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, HArrayOfByte * a4, HArrayOfByte * a5)
{
    SQLLEN len = 0;
    memset(B(a4), 0, obj_length(a4));
    B(a5)[0] = SQLGetData(H(a2), a3, SQL_C_CHAR, B(a4), obj_length(a4), &len);
    B(a5)[1] = len == SQL_NULL_DATA;
    return len;
}

void sun_jdbc_odbc_JdbcOdbc_getDataStringDate(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, HArrayOfByte * a4, HArrayOfByte * a5)
{
    DATE_STRUCT value;
    SQLLEN len = 0;
    memset(&value, 0, sizeof(value));
    B(a4)[0] = 0;
    B(a5)[0] = SQLGetData(H(a2), a3, SQL_C_DATE, &value, sizeof(value), &len);
    B(a5)[1] = len == SQL_NULL_DATA;
    if (len != SQL_NULL_DATA) {
        sprintf((char *)B(a4), "%04i-%02i-%02i", value.year, value.month, value.day);
    }
}

void sun_jdbc_odbc_JdbcOdbc_getDataStringTime(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, HArrayOfByte * a4, HArrayOfByte * a5)
{
    TIME_STRUCT value;
    SQLLEN len = 0;
    memset(&value, 0, sizeof(value));
    B(a4)[0] = 0;
    B(a5)[0] = SQLGetData(H(a2), a3, SQL_C_TIME, &value, sizeof(value), &len);
    B(a5)[1] = len == SQL_NULL_DATA;
    if (len != SQL_NULL_DATA) {
        sprintf((char *)B(a4), "%02i:%02i:%02i", value.hour, value.minute, value.second);
    }
}

void sun_jdbc_odbc_JdbcOdbc_getDataStringTimestamp(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, HArrayOfByte * a4, HArrayOfByte * a5)
{
    TIMESTAMP_STRUCT value;
    SQLLEN len = 0;
    memset(&value, 0, sizeof(value));
    B(a4)[0] = 0;
    B(a5)[0] = SQLGetData(H(a2), a3, SQL_C_TIMESTAMP, &value, sizeof(value), &len);
    B(a5)[1] = len == SQL_NULL_DATA;
    if (len != SQL_NULL_DATA) {
        sprintf((char *)B(a4), "%04i-%02i-%02i %02i:%02i:%02i", value.year, value.month, value.day, value.hour, value.minute, value.second);
        if (value.fraction) sprintf((char *)B(a4) + strlen((char *)B(a4)), ".%09li", value.fraction);
    }
}

long sun_jdbc_odbc_JdbcOdbc_getInfo(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, short a3, HArrayOfByte * a4)
{
    long v = 0;
    SQLSMALLINT len = 0;
    B(a4)[0] = SQLGetInfo(H(a2), a3, &v, sizeof(v), &len);
    return v;
}

long sun_jdbc_odbc_JdbcOdbc_getInfoShort(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, short a3, HArrayOfByte * a4)
{
    unsigned short v = 0;
    SQLSMALLINT len = 0;
    B(a4)[0] = SQLGetInfo(H(a2), a3, &v, sizeof(v), &len);
    return v;
}

void sun_jdbc_odbc_JdbcOdbc_getInfoString(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, short a3, HArrayOfByte * a4, HArrayOfByte * a5)
{
    SQLSMALLINT len = 0;
    B(a5)[0] = SQLGetInfo(H(a2), a3, B(a4), obj_length(a4), &len);
}

void sun_jdbc_odbc_JdbcOdbc_getTypeInfo(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, short a3, HArrayOfByte * a4)
{
    B(a4)[0] = SQLGetTypeInfo(H(a2), (short)a3);
}

void sun_jdbc_odbc_JdbcOdbc_moreResults(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    B(a3)[0] = SQLMoreResults(H(a2));
}

void sun_jdbc_odbc_JdbcOdbc_nativeSql(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, HArrayOfByte * a4, HArrayOfByte * a5)
{
    SQLINTEGER len = 0;
    B(a5)[0] = SQLNativeSql(H(a2), S(a3), SQL_NTS, (SQLCHAR *)B(a4), obj_length(a4), &len);
}

long sun_jdbc_odbc_JdbcOdbc_numParams(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    SQLSMALLINT v = 0;
    B(a3)[0] = SQLNumParams(H(a2), &v);
    return v;
}

long sun_jdbc_odbc_JdbcOdbc_numResultCols(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    SQLSMALLINT v = 0;
    B(a3)[0] = SQLNumResultCols(H(a2), &v);
    return v;
}

long sun_jdbc_odbc_JdbcOdbc_paramData(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    SQLPOINTER value;
    SQLRETURN status;
    long result = -1;
    status = SQLParamData(H(a2), &value);
    if (status == SQL_NEED_DATA) result = *(long *)value;
    B(a3)[0] = status;
    return result;
}

void sun_jdbc_odbc_JdbcOdbc_prepare(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, HArrayOfByte * a4)
{
    B(a4)[0] = SQLPrepare(H(a2), S(a3), SQL_NTS);
}

void sun_jdbc_odbc_JdbcOdbc_primaryKeys(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, long a4, struct Hjava_lang_String * a5, long a6, struct Hjava_lang_String * a7, long a8, HArrayOfByte * a9)
{
    SQLCHAR *s0 = NULL;
    SQLCHAR *s1 = NULL;
    SQLCHAR *s2 = NULL;
    if (!a4) s0 = S(a3);
    if (!a6) s1 = S(a5);
    if (!a8) s2 = S(a7);
    B(a9)[0] = SQLPrimaryKeys(H(a2), s0, SQL_NTS, s1, SQL_NTS, s2, SQL_NTS);
}

void sun_jdbc_odbc_JdbcOdbc_procedures(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, long a4, struct Hjava_lang_String * a5, long a6, struct Hjava_lang_String * a7, long a8, HArrayOfByte * a9)
{
    SQLCHAR *s0 = NULL;
    SQLCHAR *s1 = NULL;
    SQLCHAR *s2 = NULL;
    if (!a4) s0 = S(a3);
    if (!a6) s1 = S(a5);
    if (!a8) s2 = S(a7);
    B(a9)[0] = SQLProcedures(H(a2), s0, SQL_NTS, s1, SQL_NTS, s2, SQL_NTS);
}

void sun_jdbc_odbc_JdbcOdbc_procedureColumns(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, long a4, struct Hjava_lang_String * a5, long a6, struct Hjava_lang_String * a7, long a8, struct Hjava_lang_String * a9, long a10, HArrayOfByte * a11)
{
    SQLCHAR *s0 = NULL;
    SQLCHAR *s1 = NULL;
    SQLCHAR *s2 = NULL;
    SQLCHAR *s3 = NULL;
    if (!a4) s0 = S(a3);
    if (!a6) s1 = S(a5);
    if (!a8) s2 = S(a7);
    if (!a10) s3 = S(a9);
    B(a11)[0] = SQLProcedureColumns(H(a2), s0, SQL_NTS, s1, SQL_NTS, s2, SQL_NTS, s3, SQL_NTS);
}

void sun_jdbc_odbc_JdbcOdbc_putData(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3, long a4, HArrayOfByte * a5)
{
    B(a5)[0] = SQLPutData(H(a2), B(a3), a4);
}

long sun_jdbc_odbc_JdbcOdbc_rowCount(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, HArrayOfByte * a3)
{
    SQLLEN v = 0;
    B(a3)[0] = SQLRowCount(H(a2), &v);
    return v;
}

void sun_jdbc_odbc_JdbcOdbc_setConnectOption(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, short a3, long a4, HArrayOfByte * a5)
{
    B(a5)[0] = SQLSetConnectOption(H(a2), a3, a4);
}

void sun_jdbc_odbc_JdbcOdbc_setConnectOptionString(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, short a3, struct Hjava_lang_String * a4, HArrayOfByte * a5)
{
    B(a5)[0] = SQLSetConnectOption(H(a2), a3, (SQLULEN)S(a4));
}

void sun_jdbc_odbc_JdbcOdbc_setCursorName(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, HArrayOfByte * a4)
{
    B(a4)[0] = SQLSetCursorName(H(a2), S(a3), SQL_NTS);
}

void sun_jdbc_odbc_JdbcOdbc_setStmtOption(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, short a3, long a4, HArrayOfByte * a5)
{
    B(a5)[0] = SQLSetStmtOption(H(a2), a3, a4);
}

void sun_jdbc_odbc_JdbcOdbc_specialColumns(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, short a3, struct Hjava_lang_String * a4, long a5, struct Hjava_lang_String * a6, long a7, struct Hjava_lang_String * a8, long a9, long a10, long a11, HArrayOfByte * a12)
{
    SQLCHAR *catalog = NULL, *schema = NULL, *table = NULL;
    if (!a5) catalog = S(a4);
    if (!a7) schema = S(a6);
    if (!a9) table = S(a8);
    B(a12)[0] = SQLSpecialColumns(H(a2), a3, catalog, SQL_NTS, schema, SQL_NTS, table, SQL_NTS, a10, a11 ? 1 : 0);
}

void sun_jdbc_odbc_JdbcOdbc_statistics(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, long a4, struct Hjava_lang_String * a5, long a6, struct Hjava_lang_String * a7, long a8, long a9, long a10, HArrayOfByte * a11)
{
    SQLCHAR *catalog = NULL, *schema = NULL, *table = NULL;
    if (!a4) catalog = S(a3);
    if (!a6) schema = S(a5);
    if (!a8) table = S(a7);
    B(a11)[0] = SQLStatistics(H(a2), catalog, SQL_NTS, schema, SQL_NTS, table, SQL_NTS, a9 ? 0 : 1, a10 ? 0 : 1);
}

void sun_jdbc_odbc_JdbcOdbc_tables(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, long a4, struct Hjava_lang_String * a5, long a6, struct Hjava_lang_String * a7, long a8, struct Hjava_lang_String * a9, long a10, HArrayOfByte * a11)
{
    SQLCHAR *s0 = NULL;
    SQLCHAR *s1 = NULL;
    SQLCHAR *s2 = NULL;
    SQLCHAR *s3 = NULL;
    if (!a4) s0 = S(a3);
    if (!a6) s1 = S(a5);
    if (!a8) s2 = S(a7);
    if (!a10) s3 = S(a9);
    B(a11)[0] = SQLTables(H(a2), s0, SQL_NTS, s1, SQL_NTS, s2, SQL_NTS, s3, SQL_NTS);
}

void sun_jdbc_odbc_JdbcOdbc_tablePrivileges(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, struct Hjava_lang_String * a3, long a4, struct Hjava_lang_String * a5, long a6, struct Hjava_lang_String * a7, long a8, HArrayOfByte * a9)
{
    SQLCHAR *s0 = NULL;
    SQLCHAR *s1 = NULL;
    SQLCHAR *s2 = NULL;
    if (!a4) s0 = S(a3);
    if (!a6) s1 = S(a5);
    if (!a8) s2 = S(a7);
    B(a9)[0] = SQLTablePrivileges(H(a2), s0, SQL_NTS, s1, SQL_NTS, s2, SQL_NTS);
}

void sun_jdbc_odbc_JdbcOdbc_transact(struct Hsun_jdbc_odbc_JdbcOdbc* a1, long a2, long a3, short a4, HArrayOfByte * a5)
{
    B(a5)[0] = SQLTransact(H(a2), H(a3), a4);
}
