#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/audioio.h>
#include <sys/sensors.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <err.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <machine/apmvar.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>

#include "config.h"

static unsigned char	 battery(void);
static const char	*ip(const char *);
static unsigned int	 cputemp(void);
static const char	*timedate(void);
static unsigned int	 volume(void);

static Display *dpy;

static unsigned char
battery(void)
{
	static int fd;
	static struct apm_power_info pi;

	if ((fd = open("/dev/apm", O_RDONLY)) == -1)
		warn("open");
	if (ioctl(fd, APM_IOC_GETPOWER, &pi) == -1) {
		close(fd);
		warn("ioctl");
	}
	close(fd);

	switch (pi.battery_state) {
	case APM_BATT_UNKNOWN:
		warnx("unknown battery state");
		return (0);
	case APM_BATTERY_ABSENT:
		warnx("no battery");
		return (0);
	}
	return (pi.battery_life);
}

static const char *
ip(const char *ifn)
{
	static struct ifaddrs *ifap, *ifa;
	static const struct sockaddr_in  *sin;
	static const struct sockaddr_in6 *sin6;
	static char addr[INET6_ADDRSTRLEN];

	static const size_t addr_sz = sizeof(addr);

	if (getifaddrs(&ifap) == -1) {
		warn("getifaddrs");
		return ("");
	}

	for (ifa = ifap; ifa && strcmp(ifa->ifa_name, ifn);
	     ifa = ifa->ifa_next)
		;
	if (!ifa)
		goto fail;

	for (; ifa && ifa->ifa_addr && !strcmp(ifa->ifa_name, ifn);
	     ifa = ifa->ifa_next)
		switch (ifa->ifa_addr->sa_family) {
		case AF_INET:
			sin = (const struct sockaddr_in *)ifa->ifa_addr;
			if (inet_ntop(sin->sin_family, &sin->sin_addr,
			              addr, addr_sz))
				goto success;
			goto fail;
		case AF_INET6:
			sin6 = (const struct sockaddr_in6 *)ifa->ifa_addr;
#if SKIP_LLA
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
				continue;
#endif
			if (inet_ntop(sin6->sin6_family, &sin6->sin6_addr,
			              addr, addr_sz))
				goto success;
			goto fail;
		}
success:
	freeifaddrs(ifap);
#if SKIP_LLA
	if (!ifa && !*addr)
		warnx("cannot find IP address");
#endif
	return (addr);

fail:
	freeifaddrs(ifap);
	err(1, "freeifaddrs");
	/* unreachable */
	return ("");
}

static unsigned int
cputemp(void)
{
	static const int mib[5] = {CTL_HW, HW_SENSORS, 0, SENSOR_TEMP, 0};
	static struct sensor sn;
	static size_t sn_sz;

	sn_sz = sizeof(sn);

	if (sysctl(mib, 5, &sn, &sn_sz, NULL, 0) == -1)
		err(1, "sysctl");
	return ((sn.value - 273150000) / 1000000);
}

static const char *
timedate(void)
{
	static time_t t;
	static struct tm *tm;
	static char s[64];

	if ((t = time(NULL)) == (time_t)-1)
		err(1, "time");
	if (!(tm = localtime(&t)))
		err(1, "localtime");
	if (!strftime(s, sizeof(s), TIMEFMT, tm))
		err(1, "strftime");
	return (s);
}

static unsigned int
volume(void)
{
	static int cls;
	static struct mixer_devinfo mdi;
	static struct mixer_ctrl mc;
	static int m;
	static int v;

	if ((m = open("/dev/mixer", O_RDONLY)) == -1)
		err(1, "open");

	cls = -1;
	v = -1;

	for (mdi.index = 0; cls == -1; ++mdi.index) {
		if (ioctl(m, AUDIO_MIXER_DEVINFO, &mdi) == -1)
			err(1, "ioctl");
		if (mdi.type == AUDIO_MIXER_CLASS &&
		    !strcmp(mdi.label.name, AudioCoutputs))
			cls = mdi.index;
	}
	for (mdi.index = 0; v == -1; ++mdi.index) {
		if (ioctl(m, AUDIO_MIXER_DEVINFO, &mdi) == -1)
			err(1, "ioctl");
		if (mdi.type == AUDIO_MIXER_VALUE &&
		    mdi.prev == AUDIO_MIXER_LAST &&
		    mdi.mixer_class == cls &&
		    !strcmp(mdi.label.name, AudioNmaster)) {
			mc.dev = mdi.index;
			if (ioctl(m, AUDIO_MIXER_READ, &mc) == -1)
				err(1, "ioctl");
			v = mc.un.value.num_channels == 1 ?
			    mc.un.value.level[AUDIO_MIXER_LEVEL_MONO] :
			    MAX(mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT],
			        mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT]);
		}
	}

	if (v == -1)
		errx(1, "cannot get system volume");
	return (v * 100 / 255);
}

static void
setstatus(const char *fmt, ...)
{
	static char s[MAX_LEN];
	static va_list ap;
	static int r;

	static const size_t s_sz = sizeof(s);
	va_start(ap, fmt);

	if ((r = vsnprintf(s, s_sz, fmt, ap)) == -1)
		err(1, "vsnprintf");
	if ((size_t)r >= s_sz)
		warn("vsnprintf");

	va_end(ap);

	XStoreName(dpy, DefaultRootWindow(dpy), s);
	XSync(dpy, False);
}

int
main(void)
{
	if (!(dpy = XOpenDisplay(NULL)))
		errx(1, "cannot open display");

loop:
	setstatus(OUTFMT,
	          ip(INTERFACE),
	          battery(),
	          cputemp(),
	          volume(),
	          timedate());
	(void)sleep(INTERVAL);
	goto loop;

	XCloseDisplay(dpy);
	return (0);
}
