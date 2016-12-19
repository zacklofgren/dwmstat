#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

static Display *dpy;

static const char*
_time(void)
{
	time_t t;
	struct tm *tm;
	static char s[26];

	if ((t = time(NULL)) == (time_t)-1)
		errx(1, "could not get current system time");
	if (!(tm = localtime(&t)))
		errx(1, "could not convert system to local time");
	if (!strftime(s, sizeof(s), "%a %d.%m.%y %H:%M", tm))
		errx(1, "could not convert local time to string");

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

static void
setstatus(const char *s)
{
	XStoreName(dpy, DefaultRootWindow(dpy), s);
	XSync(dpy, False);
}

int
main(void)
{
	if (!(dpy = XOpenDisplay(NULL)))
		errx(1, "cannot open display");

	printf("%3d%%  %s",
	       _volume(),
	       _time());

	XCloseDisplay(dpy);

	return 0;
}
