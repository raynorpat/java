#include <stdio.h>
void BootstrapFixture_AInitialization(void) { puts("versioned initializer"); fflush(stdout); }
void BootstrapFallbackInitialization(void) { puts("fallback initializer"); fflush(stdout); }
