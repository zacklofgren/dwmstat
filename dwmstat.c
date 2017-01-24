#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/audioio.h>
#include <sys/sensors.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <machine/apmvar.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <X11/Xlib.h>

#include "config.h"

static Display *dpy;
static char stat[MAX_LEN];

static char		 battery(void);
static char		 cputemp(void);
static const char	*ip(void);
static const char	*timedate(void);
static char		 volume(void);
static void		 handler(int);

static char
battery(void)
{
	static int fd;
	static struct apm_power_info pi;

	if ((fd = open("/dev/apm", O_RDONLY)) == -1) {
		warn("open");
		return (-1);
	}
	if (ioctl(fd, APM_IOC_GETPOWER, &pi) == -1) {
		warn("ioctl");
		if (close(fd) == -1)
			warn("close");
		return (-1);
	}
	if (close(fd) == -1) {
		warn("close");
		return (-1);
	}

	switch (pi.battery_state) {
	case APM_BATT_UNKNOWN:
		warnx("unknown battery state");
		return (-1);
	case APM_BATTERY_ABSENT:
		warnx("no battery");
		return (-1);
	}
	return (char)(pi.battery_life);
}

static char
cputemp(void)
{
	static const int mib[5] = {CTL_HW, HW_SENSORS, 0, SENSOR_TEMP, 0};
	static struct sensor sn;
	static size_t sn_sz;

	sn_sz = sizeof(sn);

	if (sysctl(mib, 5, &sn, &sn_sz, NULL, 0) == -1) {
		warn("sysctl");
		return (-1);
	}
	return ((sn.value - 273150000LL) / 1000000LL);
}

static const char *
ip(void)
{
	static struct ifaddrs *ifap, *ifa;
	static const struct sockaddr_in  *sin;
	static const struct sockaddr_in6 *sin6;
	static char addr[INET6_ADDRSTRLEN];

	if (getifaddrs(&ifap) == -1) {
		warn("getifaddrs");
		return ("!");
	}
	for (ifa = ifap; ifa && strcmp(ifa->ifa_name, INTERFACE) != 0;
	     ifa = ifa->ifa_next)
		;
	if (!ifa) {
		warnx("no such interface");
		goto fail;
	}

	for (; ifa && ifa->ifa_addr && strcmp(ifa->ifa_name, INTERFACE) == 0;
	     ifa = ifa->ifa_next)
		if (ifa->ifa_addr->sa_family == AF_INET6) {
			sin6 = (const struct sockaddr_in6 *)ifa->ifa_addr;
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
				continue;
			if (inet_ntop(AF_INET6, &sin6->sin6_addr,
			              addr, INET6_ADDRSTRLEN))
				break;
			warn("inet_ntop");
			goto fail;
		} else if (ifa->ifa_addr->sa_family == AF_INET) {
			sin = (const struct sockaddr_in *)ifa->ifa_addr;
			if (inet_ntop(AF_INET, &sin->sin_addr,
			              addr, INET_ADDRSTRLEN))
				break;
			warn("inet_ntop");
			goto fail;
		}
	freeifaddrs(ifap);
	if (ifa)
		return (addr);
	warnx("no IP");
	return ("-");
fail:
	freeifaddrs(ifap);
	return ("!");
}

static const char *
timedate(void)
{
	static time_t t;
	static struct tm *tm;
	static char s[64];

	if ((t = time(NULL)) == (time_t)-1) {
		warn("time");
		return ("!");
	}
	if (!(tm = localtime(&t))) {
		warn("localtime");
		return ("!");
	}
	if (strftime(s, sizeof(s), TIMEFMT, tm) == 0) {
		warn("strftime");
		return ("!");
	}
	return (s);
}

static char
volume(void)
{
	static int cls;
	static struct mixer_devinfo mdi;
	static struct mixer_ctrl mc;
	static int fd;
	static int v;

	if ((fd = open("/dev/mixer", O_RDONLY)) == -1) {
		warn("open");
		return (-1);
	}

	cls = -1;
	v = -1;

	for (mdi.index = 0; cls == -1; ++mdi.index) {
		if (ioctl(fd, AUDIO_MIXER_DEVINFO, &mdi) == -1)
			goto fail;
		if (mdi.type == AUDIO_MIXER_CLASS &&
		    strcmp(mdi.label.name, AudioCoutputs) == 0)
			cls = mdi.index;
	}
	for (mdi.index = 0; v == -1; ++mdi.index) {
		if (ioctl(fd, AUDIO_MIXER_DEVINFO, &mdi) == -1)
			goto fail;
		if (mdi.type == AUDIO_MIXER_VALUE &&
		    mdi.prev == AUDIO_MIXER_LAST &&
		    mdi.mixer_class == cls &&
		    strcmp(mdi.label.name, AudioNmaster) == 0) {
			mc.dev = mdi.index;
			if (ioctl(fd, AUDIO_MIXER_READ, &mc) == -1)
				goto fail;
			v = mc.un.value.num_channels == 1 ?
			    mc.un.value.level[AUDIO_MIXER_LEVEL_MONO] :
			    MAX(mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT],
			        mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT]);
		}
	}

	if (v == -1) {
		warnx("cannot get system volume");
		return (-1);
	}
	if (close(fd) == -1)
		warn("close");
	return (v * 100 / 255);
fail:
	warn("ioctl");
	if (close(fd) == -1)
		warn("close");
	return (-1);
}

static void
handler(const int sig)
{
	int save_errno = errno;

	psignal((unsigned int)sig, "caught signal");
	switch (sig) {
	case SIGABRT:
	case SIGTERM:
		(void)XCloseDisplay(dpy);
		exit(0);
	case SIGALRM:
	case SIGHUP:
		break;
	case SIGINFO:
		if (puts(stat) < 0)
			warn("puts");
		break;
	}

	errno = save_errno;
}

int
main(void)
{
	static int r;

	if (!(dpy = XOpenDisplay(NULL)))
		errx(1, "cannot open display");

	signal(SIGABRT, handler);
	signal(SIGALRM, handler);
	signal(SIGHUP,  handler);
	signal(SIGINFO, handler);
	signal(SIGTERM, handler);

loop:
	if ((r = snprintf(stat, MAX_LEN, OUTFMT, ip(), battery(),
	                  cputemp(), volume(), timedate())) == -1)
		warn("vsnprintf");
	else if (r >= MAX_LEN)
		warnx("status exceeds MAX_LEN");
	else {
		(void)XStoreName(dpy, DefaultRootWindow(dpy), stat);
		(void)XSync(dpy, False);
	}
	(void)sleep(INTERVAL);
	goto loop;

	/* unreachable */
	return (0);
}
