#if defined(WIN32)
#  include <winsock.h>
#else
#  include <arpa/inet.h>
#  include <endian.h>
#  define ntohll(x) be64toh(x)
#endif
