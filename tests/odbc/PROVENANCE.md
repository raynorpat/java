# JDBC-ODBC 1.1.5 reconstruction evidence

The unchanged Sun Windows binaries were read through IDA. Addresses below map
each generated legacy stub to its native implementation. Local variable names
are reconstructed. The classes.zip javah header and stubs are checked in beside
odbc115.c; no vendor binaries are installed by the target.

- JdbcOdbc.dll SHA-256: `fe1b7cb01978c4db20caf0e425f6b8b5031fd94abe0b00028c8b1b36dbe2dd7a`
- JdbcOdbc_g.dll SHA-256: `6f7381b57d5fd54b23c4363577cf7066e0b6145b1a430eebe7fdc40d24ae6789`

Reproduce signatures with the reconstructed javah against the original archive:

```
javah -classpath vendor/jdk1.1.5/lib/classes.zip sun.jdbc.odbc.JdbcOdbc
javah -stubs -classpath vendor/jdk1.1.5/lib/classes.zip sun.jdbc.odbc.JdbcOdbc
```

| Native method | Release body | Debug body | Driver manager calls |
|---|---|---|---|
| allocConnect | 0x10001010 | 0x10001037 | SQLAllocConnect |
| allocEnv | 0x10001040 | 0x10001075 | SQLAllocEnv |
| allocStmt | 0x10001070 | 0x100010af | SQLAllocStmt |
| bindColDefault | 0x100010a0 | 0x100010ed | SQLBindCol |
| bindInParameterAtExec | 0x100010e0 | 0x10001146 | SQLBindParameter |
| bindInParameterBinary | 0x10001160 | 0x1000121a | SQLBindParameter |
| bindInParameterDate | 0x10001200 | 0x100012f4 | SQLBindParameter |
| bindInParameterDouble | 0x10001280 | 0x10001390 | SQLBindParameter |
| bindInParameterFloat | 0x100012d0 | 0x1000140b | SQLBindParameter |
| bindInParameterInteger | 0x10001320 | 0x10001486 | SQLBindParameter |
| bindInParameterNull | 0x10001360 | 0x10001501 | SQLBindParameter |
| bindInParameterString | 0x100013c0 | 0x1000157f | SQLBindParameter |
| bindInParameterTime | 0x10001450 | 0x1000163b | SQLBindParameter |
| bindInParameterTimestamp | 0x100014d0 | 0x100016d7 | SQLBindParameter |
| bindOutParameterString | 0x10001580 | 0x1000178e | SQLBindParameter |
| browseConnect | 0x100015d0 | 0x100017f6 | SQLBrowseConnect |
| bufferToInt | 0x10001000 | 0x10001000 |  |
| cancel | 0x10001620 | 0x1000185d | SQLCancel |
| colAttributesString | 0x10001680 | 0x100018dc | SQLColAttributes |
| colAttributes | 0x10001640 | 0x10001888 | SQLColAttributes |
| columnPrivileges | 0x10001770 | 0x100019fb | SQLColumnPrivileges |
| columns | 0x100016d0 | 0x10001938 | SQLColumns |
| describeParam | 0x10001810 | 0x10001abe | SQLDescribeParam |
| disconnect | 0x100018a0 | 0x10001b77 | SQLDisconnect |
| driverConnect | 0x100018c0 | 0x10001ba2 | SQLDriverConnect |
| error | 0x10001910 | 0x10001bff | SQLError |
| execDirect | 0x10001970 | 0x10001c78 | SQLExecDirect |
| execute | 0x100019a0 | 0x10001cb8 | SQLExecute |
| fetch | 0x100019c0 | 0x10001ce3 | SQLFetch |
| foreignKeys | 0x100019e0 | 0x10001d0e | SQLForeignKeys |
| freeConnect | 0x10001ad0 | 0x10001e1d | SQLFreeConnect |
| freeEnv | 0x10001af0 | 0x10001e48 | SQLFreeEnv |
| freeStmt | 0x10001b10 | 0x10001e73 | SQLFreeStmt |
| getConnectOptionString | 0x10001b70 | 0x10001ee4 | SQLGetConnectOption |
| getConnectOption | 0x10001b30 | 0x10001ea2 | SQLGetConnectOption |
| getCursorName | 0x10001ba0 | 0x10001f1f | SQLGetCursorName |
| getDataBinary | 0x10001be0 | 0x10001f71 | SQLGetData |
| getDataDouble | 0x10001c60 | 0x10002007 | SQLGetData |
| getDataFloat | 0x10001cc0 | 0x10002077 | SQLGetData |
| getDataInteger | 0x10001d20 | 0x100020e0 | SQLGetData |
| getDataStringDate | 0x10001df0 | 0x100021d3 | SQLGetData |
| getDataStringTime | 0x10001e90 | 0x10002277 | SQLGetData |
| getDataStringTimestamp | 0x10001f30 | 0x1000231f | SQLGetData |
| getDataString | 0x10001d80 | 0x10002149 | SQLGetData |
| getInfoShort | 0x10002040 | 0x10002456 | SQLGetInfo |
| getInfoString | 0x10002090 | 0x100024a8 | SQLGetInfo |
| getInfo | 0x10002000 | 0x10002408 | SQLGetInfo |
| getStmtOption | 0x100020d0 | 0x100024fe | SQLGetStmtOption |
| getTypeInfo | 0x10002110 | 0x10002540 | SQLGetTypeInfo |
| moreResults | 0x10002130 | 0x10002576 | SQLMoreResults |
| nativeSql | 0x10002150 | 0x100025a1 | SQLNativeSql |
| numParams | 0x100021a0 | 0x10002608 | SQLNumParams |
| numResultCols | 0x100021d0 | 0x10002646 | SQLNumResultCols |
| paramData | 0x10002200 | 0x10002684 | SQLParamData |
| prepare | 0x10002240 | 0x100026e1 | SQLPrepare |
| primaryKeys | 0x10002270 | 0x10002721 | SQLPrimaryKeys |
| procedureColumns | 0x10002350 | 0x1000285b | SQLProcedureColumns |
| procedures | 0x100022e0 | 0x100027be | SQLProcedures |
| putData | 0x100023f0 | 0x1000291e | SQLPutData |
| rowCount | 0x10002420 | 0x10002959 | SQLRowCount |
| setConnectOptionString | 0x10002470 | 0x100029ca | SQLSetConnectOption |
| setConnectOption | 0x10002450 | 0x10002997 | SQLSetConnectOption |
| setCursorName | 0x100024a0 | 0x10002a0c | SQLSetCursorName |
| setStmtOption | 0x100024d0 | 0x10002a4c | SQLSetStmtOption |
| specialColumns | 0x100024f0 | 0x10002a7f | SQLSpecialColumns |
| statistics | 0x10002590 | 0x10002b3e | SQLStatistics |
| tablePrivileges | 0x100026e0 | 0x10002cd2 | SQLTablePrivileges |
| tables | 0x10002640 | 0x10002c0f | SQLTables |
| transact | 0x10002750 | 0x10002d6f | SQLTransact |

## Recovered contracts

- Java handles and ODBC handles occupy 32-bit stack slots; javah handles double two-slot arguments and floating return values. The DLL exports only the 69 legacy stubs.
- Status arrays contain the low byte of SQLRETURN. Data getters additionally write byte 1 for SQL_NULL_DATA. String retrieval clears its whole output array, returns the full ODBC length, and preserves SQL_SUCCESS_WITH_INFO on truncation. Binary retrieval casts requested capacity to unsigned short and clamps SQL_NO_TOTAL/oversize lengths to that requested capacity, as Sun did.
- ODBC parameters and bound columns retain direct pointers into Java byte-array bodies. The original Java bridge keeps these arrays reachable. Binary and string inputs copy to caller-owned parameter buffers; numeric and date/time inputs use the corresponding 32-bit ODBC layouts. At-exec binding writes the parameter number and SQL_LEN_DATA_AT_EXEC indicator into caller buffers.
- The original bridge does not pin parameter buffers. Experimentally, forcing System.gc() between bind and execute corrupts bound addresses on the original Sun VM as well (Access reports invalid datetime). The reconstruction preserves this limitation; it does not claim forced-GC-safe native bindings.
- The date/time formatting literals are %04i-%02i-%02i, %02i:%02i:%02i, and timestamp with optional .%09li fraction. They are read from the binary, including release address 0x10006050 for the fractional format.

## Test fixture and limits

Inventory was taken with Get-OdbcDriver -Platform 32-bit before fixture selection. The machine has Microsoft Access, dBase, Text, Excel, Paradox, SQL Server and Oracle drivers (localized aliases included). Tests use the installed 32-bit Microsoft Access (*.mdb) driver and Jet 4.0 ADOX to create disposable MDBs. No global/user DSN is created or modified by the harness. The Access driver may itself create its documented volatile process DSN.

The original 1.1.5 JdbcOdbcConnection Java class prefixes every connection string with DSN=. The Java-only fixture subclass strips that prefix before delegating to the original native SQLDriverConnect, enabling an isolated DSN-less MDB while keeping original Connection, PreparedStatement, ResultSet and DatabaseMetaData implementations. Direct-native tests use the native DSN-less string without this adapter.

JdbcOdbcAbi115 exercises every one of the 69 entries against the real manager, asserting allocation/cleanup, IM002 in the JDBC test, invalid-handle statuses, integer/float/date/string buffers and NULL/at-exec indicators. Unsupported Access features (including output parameters, privilege/procedure catalog calls) have ABI/error coverage rather than invented success behavior. JdbcOdbc115 covers real prepared insertion, metadata, SQL NULL, commit and rollback. JdbcOdbcData115 covers native binary/float/double/date/time/timestamp conversions, parameter buffers retained across execution, default bound columns, truncation length/content/status, and data-at-exec streaming.

The Java wrapper passes DEFAULT_IN_PRECISION for binary binding; current Access rejects that large precision for BINARY(4), including with the original Sun library. The direct fixture supplies precision 4 through the original protected native method. This is a test adaptation, not a change to bridge behavior.

The CTest harness compiles the fixtures with the reconstructed 1.1.5 VM/compiler classes, runs original and rebuilt libraries separately, fails on any exception or stderr, and compares stdout. Debug selects the original java_g.exe/JdbcOdbc_g.dll. Export checks separately enforce the exact manifest. Per-run driver inventories and stdout/stderr are retained below the build directory.
