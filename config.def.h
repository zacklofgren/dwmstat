/* interface to show IP addresses for */
#define INTERFACE "trunk0"
/* seconds to wait between polling */
#define INTERVAL  60
/* maximum number of characters in status string */
#define MAX_LEN   128
/* output format used by printf(3) for status string */
#define OUTFMT    "%s  %3d%%  %3d°C  %3d%%  %s"
/* skip IPv6 link-local addresses when scanning INTERFACE */
#define SKIP_LLA  1
/* time format used by strftime(3) within status string */
#define TIMEFMT   "%a %d.%m.%y %H:%M"
