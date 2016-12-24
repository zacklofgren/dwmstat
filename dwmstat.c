#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <ifaddrs.h>
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

static Display *dpy;

static void
setstatus(const char *s)
{
	XStoreName(dpy, DefaultRootWindow(dpy), s);
	XSync(dpy, False);
}

static const char*
_ip(const char* ifn)
{
	struct ifaddrs *ifap, *ifa;
	static char addr[32];

	if (getifaddrs(&ifap) == -1)
		errx(1, "could not get network interfaces list");

	for (ifa = ifap; ifa; ifa = ifa->ifa_next)
		if (!strcmp(ifa->ifa_name, ifn))
			break;
	if (!ifa) {
		freeifaddrs(ifap);
		errx(1, "could not find given interface");
	}

	for (; ifa && !strcmp(ifa->ifa_name, ifn); ifa = ifa->ifa_next)
		if (ifa->ifa_addr &&
		    (ifa->ifa_addr->sa_family == AF_INET ||
		     ifa->ifa_addr->sa_family == AF_INET6)) {
			if (inet_ntop(ifa->ifa_addr->sa_family,
			              ifa->ifa_addr->sa_data,
			              addr,
			              sizeof(addr)))
				break;
			else
				warnx("could not convert IP address");
		}
	freeifaddrs(ifap);

	if (!ifa)
		errx(1, "could not find IP address");
	return addr;
}

static const unsigned int
_temp(void)
{
	static int mib[5];
	struct sensor sn;
	size_t sn_sz;

	mib[0] = CTL_HW;
	mib[1] = HW_SENSORS;
	mib[2] = 0;
	mib[3] = SENSOR_TEMP;
	mib[4] = 0;
	sn_sz = sizeof(sn);

	if (sysctl(mib, 5, &sn, &sn_sz, NULL, 0) == -1)
		errx(1, "could not read CPU temperature");
	return (sn.value - 273150000) / 1000000;
}

static const char*
_time(void)
{
	time_t t;
	struct tm *tm;
	static char s[26];

	if ((t = time(NULL)) == (time_t)-1 ||
	    !(tm = localtime(&t)) ||
	    !strftime(s, sizeof(s), "%a %d.%m.%y %H:%M", tm))
		errx(1, "could not get current system time");
	return s;
}

static const unsigned int
_volume(void)
{
	static int cls;
	struct mixer_devinfo mdi;
	struct mixer_ctrl mc;
	int m;
	int v;

	if ((m = open("/dev/mixer", O_RDONLY)) == -1)
		errx(1, "could not open mixer device");

	cls = -1;
	v = -1;

	for (mdi.index = 0; cls == -1; ++mdi.index) {
		if (ioctl(m, AUDIO_MIXER_DEVINFO, &mdi) == -1)
			errx(1, "could not get audio mixer information");
		if (mdi.type == AUDIO_MIXER_CLASS &&
		    !strcmp(mdi.label.name, AudioCoutputs))
			cls = mdi.index;
	}
	for (mdi.index = 0; v == -1; ++mdi.index) {
		if (ioctl(m, AUDIO_MIXER_DEVINFO, &mdi) == -1)
			errx(1, "could not get audio mixer information");
		if (mdi.type == AUDIO_MIXER_VALUE &&
		    mdi.prev == AUDIO_MIXER_LAST &&
		    mdi.mixer_class == cls &&
		    !strcmp(mdi.label.name, AudioNmaster)) {
			mc.dev = mdi.index;
			if (ioctl(m, AUDIO_MIXER_READ, &mc) == -1)
				errx(1, "could not read audio mixer");
			v = mc.un.value.num_channels == 1 ?
			    mc.un.value.level[AUDIO_MIXER_LEVEL_MONO] :
			    MAX(mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT],
			        mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT]);
		}
	}

	if (v == -1)
		errx(1, "could not get system volume");
	return (v * 100) / 255;
}

int
main(void)
{
	if (!(dpy = XOpenDisplay(NULL)))
		errx(1, "cannot open display");

	printf("%s  %3d%%  %3d°C  %s\n",
	       _ip("trunk0"),
	       _volume(),
	       _temp(),
	       _time());

	XCloseDisplay(dpy);

	return 0;
}
