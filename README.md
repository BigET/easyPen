# easyPen
User space driver for easy pen genius graphic tablet.

There are out there old graphic tablets,
the ones that are using probably serial port.
They are still in good shape.

ALAS. On a modern linux os (like ubuntu 16.04) they cannot be used.
You can go out there and try to use inputattach, but in my case it was not working.

This is because besides inputattach you have to have a kernel driver to do
the heavy lifting.

In the old days there was an X.org driver that now is no longer maintained and
since the X.org has moved with the protocol it no longer compiles.

That driver was named *summa*.

Instead of trying to maintain something that will work only on X11,
I decided to port the logic of that driver to the uinput layer of linux.
In this way we will get a more consistent usage.
And I didn't wanted to learn X11 driver protocol.

The uinput layer is nice since you can write your input device driver as a
userspace application. It also supports the absolute pointer mode.
Perfect for a tablet input.

I have butcher the logic from the xorg-x11-drv-summa driver.
For the first stage I just want my genius easy pen to work.

After that I will try to hit 100% compatibility with the summa driver.

Compilation instructions.

```
make easypen_input
```

To use it just run it.

```
easypen_input {ttyS dev file}
```
