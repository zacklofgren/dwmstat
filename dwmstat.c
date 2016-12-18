#include <err.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

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

	setstatus(_time());

	XCloseDisplay(dpy);

	return 0;
}
