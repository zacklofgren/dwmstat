dwmstat
=======
dwmstat is the successor of
[setdwmstatus](https://notabug.org/kl3/scripts/setdwmstatus) in order to
provide a really simple and more efficient way of having a status bar for
[dwm](http://dwm.suckless.org).

No fancy unicode "graphics" are used, although users are free to modify
`OUTFMT` in their `config.h`.

As of now, the current IPv4 or IPv6 address, system volume, CPU temperature
as well as the current locale's time is shown.

dwmstat currently supports OpenBSD, patches (to include other operating
systems) are welcome.

Requirements
------------
In order to build dwmstat you need the Xlib header files.

Installation
------------
Edit `config.mk` to match your local setup (dwmstat is installed into the
`/usr/local` namespace by default).

Create `config.h` from or modify `config.def.h` to suit your needs.

Build and install dwmstat (if necessary as root):

		$ make clean install

Running dwmstat
---------------
dwmstat is usually started in background through xinit(1), see my
[xinitrc(5)](https://notabug.org/kl3/dotfiles/.xinitrc) for example.
