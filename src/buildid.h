#ifndef BUILDID
#define BUILDID

#define VERSION_NAME "Beleriand"

#ifdef BUILD_ID
# define STR(x) #x
# define XSTR(x) STR(x)
# define VERSION_STRING XSTR(BUILD_ID)
#else
# define VERSION_STRING "1.0.0"
#endif

extern const char *buildid;
extern const char *buildver;

#endif /* BUILDID */
