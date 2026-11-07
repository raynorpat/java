/* Test-only subset; signatures checked separately against the DR2 SDK. */
#ifndef TEST_DYLD_H
#define TEST_DYLD_H
typedef void *NSObjectFileImage;
typedef void *NSModule;
typedef void *NSSymbol;
typedef enum { NSObjectFileImageFailure, NSObjectFileImageSuccess,
               NSObjectFileImageInappropriateFile } NSObjectFileImageReturnCode;
NSObjectFileImageReturnCode NSCreateObjectFileImageFromFile(const char *, NSObjectFileImage *);
NSModule NSLinkModule(NSObjectFileImage, const char *, int);
int NSAddLibrary(const char *);
int NSIsSymbolNameDefined(const char *);
NSSymbol NSLookupAndBindSymbol(const char *);
void *NSAddressOfSymbol(NSSymbol);
#endif
