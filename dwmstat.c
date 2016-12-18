#include <err.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>

static Display *dpy;

void
setstatus(const char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

int
main(void)
{
	if (!(dpy = XOpenDisplay(NULL)))
		errx(1, "cannot open display");

	XCloseDisplay(dpy);

	return 0;
}
