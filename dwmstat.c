#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/audioio.h>
#include <sys/param.h>
#include <sys/sensors.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <X11/Xlib.h>

#include "config.h"

static Display *dpy;

static void
setstatus(const char *s)
{
	XStoreName(dpy, DefaultRootWindow(dpy), s);
	XSync(dpy, False);
}

static const char*
_ip(const char *ifn)
{
	static struct ifaddrs *ifap, *ifa;
	const static struct sockaddr_in *sin;
	const static struct sockaddr_in6 *sin6;
	static char addr[INET6_ADDRSTRLEN];
	const static size_t addr_sz = sizeof(addr);

	if (getifaddrs(&ifap) == -1)
		errx(1, "cannot get network interfaces list");

	for (ifa = ifap; ifa; ifa = ifa->ifa_next)
		if (!strcmp(ifa->ifa_name, ifn))
			break;
	if (!ifa) {
		freeifaddrs(ifap);
		errx(1, "cannot find given interface");
	}

	for (; ifa && !strcmp(ifa->ifa_name, ifn); ifa = ifa->ifa_next)
		if (ifa->ifa_addr)
			switch (ifa->ifa_addr->sa_family) {
			case AF_INET:
				sin = (const struct sockaddr_in *)(ifa->ifa_addr);
				if (inet_ntop(sin->sin_family, &sin->sin_addr,
				              addr, addr_sz))
					goto skip;
				else
					warnx("cannot convert IPv4 address");
				break;
			case AF_INET6:
				sin6 = (const struct sockaddr_in6 *)(ifa->ifa_addr);
				if (inet_ntop(sin6->sin6_family, &sin6->sin6_addr,
				              addr, addr_sz)) {
#ifdef NO_LLA
					if (strncmp(addr, "fe80", 4))
						goto skip;
#endif
				} else
					warnx("cannot convert IPv6 address");
				break;
			}
skip:
	freeifaddrs(ifap);

	if (!ifa)
		errx(1, "cannot find IP address");
	return (addr);
}

static const unsigned int
_temp(void)
{
	static const int mib[5] = {CTL_HW, HW_SENSORS, 0, SENSOR_TEMP, 0};
	static struct sensor sn;
	static size_t sn_sz;

	sn_sz = sizeof(sn);

	if (sysctl(mib, 5, &sn, &sn_sz, NULL, 0) == -1)
		errx(1, "cannot read CPU temperature");
	return ((sn.value - 273150000) / 1000000);
}

static const char*
_time(void)
{
	static time_t t;
	static struct tm *tm;
	static char s[64];

	if ((t = time(NULL)) == (time_t)-1 ||
	    !(tm = localtime(&t)))
		errx(1, "cannot get current system time");
	if (!strftime(s, sizeof(s), TIMEFMT, tm))
		errx(1, "cannot format system time");

	return (s);
}

static const unsigned int
_volume(void)
{
	static int cls;
	static struct mixer_devinfo mdi;
	static struct mixer_ctrl mc;
	static int m;
	static int v;

	if ((m = open("/dev/mixer", O_RDONLY)) == -1)
		errx(1, "cannot open mixer device");

	cls = -1;
	v = -1;

	for (mdi.index = 0; cls == -1; ++mdi.index) {
		if (ioctl(m, AUDIO_MIXER_DEVINFO, &mdi) == -1)
			errx(1, "cannot get audio mixer information");
		if (mdi.type == AUDIO_MIXER_CLASS &&
		    !strcmp(mdi.label.name, AudioCoutputs))
			cls = mdi.index;
	}
	for (mdi.index = 0; v == -1; ++mdi.index) {
		if (ioctl(m, AUDIO_MIXER_DEVINFO, &mdi) == -1)
			errx(1, "cannot get audio mixer information");
		if (mdi.type == AUDIO_MIXER_VALUE &&
		    mdi.prev == AUDIO_MIXER_LAST &&
		    mdi.mixer_class == cls &&
		    !strcmp(mdi.label.name, AudioNmaster)) {
			mc.dev = mdi.index;
			if (ioctl(m, AUDIO_MIXER_READ, &mc) == -1)
				errx(1, "cannot read audio mixer");
			v = mc.un.value.num_channels == 1 ?
			    mc.un.value.level[AUDIO_MIXER_LEVEL_MONO] :
			    MAX(mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT],
			        mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT]);
		}
	}

	if (v == -1)
		errx(1, "cannot get system volume");
	return ((v * 100) / 255);
}

int
main(void)
{
	if (!(dpy = XOpenDisplay(NULL)))
		errx(1, "cannot open display");

	printf(OUTFMT,
	       _ip(INTERFACE),
	       _temp(),
	       _volume(),
	       _time());

	XCloseDisplay(dpy);

	return (0);
}
