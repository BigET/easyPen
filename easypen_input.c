#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define nrToStr(x) #x
#define toStr(x) nrToStr(x)

#define die(str, args...) do { \
        perror(str "; at line " toStr(__LINE__)); \
        exit(EXIT_FAILURE); \
    } while(0)


int main(int argc, char ** argv) {
    int evfd, dfd;
    int lastX, lastY, lastLeft, lastRight, maxX, maxY, resolution = 500 /*lines per inch*/;

    {
        struct termios tio;
	char buffer[256] = "          ";
        const char initBuffer[] = "0" /*tablet id*/ "b" /*origin upper left*/
            "zb" /*binary format*/ "@" /*stream mode*/
            "I" /*increment*/ " " /* value 32 = increment 0 */
            "F" /*absolute, use E for relative*/;
	int readed;
        dfd = open(argc < 2 ? "/dev/ttyS0" : argv[1], O_RDWR | O_NOCTTY );
        if (dfd < 0) die ("error: tty open");

        bzero(&tio, sizeof(tio));
        cfmakeraw(&tio);
        tio.c_cflag |= B9600 | CS8 | CREAD | CLOCAL | HUPCL | PARENB | PARODD;
        tio.c_iflag |= IGNPAR;
        tio.c_cc[VTIME]    = 2;
        tio.c_cc[VMIN]     = 5;
        tcflush(dfd, TCIFLUSH);
        if (tcsetattr(dfd,TCSANOW,&tio) < 0) die("error: tcsetattr");
        // Send spaces to get auto-baud.
	write(dfd, buffer, 11);
        // Wait the device to settle.
        usleep(400000);
        // Put the device in prompt mode to configure it.
        if (write(dfd, "B", 1) == -1) die("error: prompt mode");
        tcflush(dfd, TCIFLUSH);
        // Read the firmware id.
        //if (write(dfd, "z?", 2) < 2) die("error: get firmware id.");
	//readed = read(dfd, buffer, 255);
	//if (readed < 0) die("error: read firmware id.");
        //buffer[readed] = '\0';
        //printf("Found tablet: %s\n", buffer);
        // Read max coordinates.
        if (write(dfd, "a", 1) < 0) die("error: get max coordinates.");
        if ((readed = read(dfd, buffer, 5)) < 5) die("error: read max coordinates.");
        maxX = (buffer[1] & 0x7f) | (buffer[2] << 7);
        maxY = (buffer[3] & 0x7f) | (buffer[4] << 7);
        printf("tablet size: %d.%02d\" x %d.%02d\" => %dx%d\n",
                maxX/resolution, 100 * maxX / resolution % 100,
                maxY/resolution, 100 * maxY / resolution % 100,
                maxX, maxY);
        // Init the tablet.
        if (write(dfd, initBuffer, 8) == -1) die("error: set options");
    }

    {
        struct uinput_user_dev uidev;

        if ((evfd = open("/dev/input/uinput", O_WRONLY | O_NONBLOCK)) < 0) {
	    if ((evfd = open("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0) die("error: open");
	}

        if (ioctl(evfd, UI_SET_EVBIT, EV_KEY) < 0) die("error: ioctl");
        if (ioctl(evfd, UI_SET_KEYBIT, BTN_LEFT) < 0) die("error: ioctl");
        if (ioctl(evfd, UI_SET_KEYBIT, BTN_RIGHT) < 0) die("error: ioctl");

        if (ioctl(evfd, UI_SET_EVBIT, EV_ABS) < 0) die("error: ioctl");
        if (ioctl(evfd, UI_SET_ABSBIT, ABS_X) < 0) die("error: ioctl");
        if (ioctl(evfd, UI_SET_ABSBIT, ABS_Y) < 0) die("error: ioctl");

        memset(&uidev, 0, sizeof(uidev));
        snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Genius Easy Pen");
        uidev.id.bustype = BUS_RS232;
        uidev.id.vendor  = 0x1;
        uidev.id.product = 0x1;
        uidev.id.version = 1;
        uidev.absmin[ABS_X] = 0;
        uidev.absmax[ABS_X] = maxX;
        uidev.absmin[ABS_Y] = 0;
        uidev.absmax[ABS_Y] = maxY;

        if(write(evfd, &uidev, sizeof(uidev)) < 0) die("error: write");
        if(ioctl(evfd, UI_DEV_CREATE) < 0) die("error: ioctl");
    }

    sleep(2);

    while(1) {
        unsigned char readBuffer [5];
        struct timeval evtime;
        int nowX, nowY, nowLeft, nowRight, evNr;
        struct input_event sendev[4];
        do {
            if (read(dfd, readBuffer, 1) < 0) die("error: device read");
        } while (0 == (readBuffer[0] & 0x80));

        if (4 != read(dfd, readBuffer + 1, 4)) continue;
        nowLeft = 0 != (readBuffer[0] & 1);
        nowRight = 0 != (readBuffer[0] & 2);
        nowX = (readBuffer[1] & 0x7f) | (readBuffer[2] << 7);
        nowY = (readBuffer[3] & 0x7f) | (readBuffer[4] << 7);
        gettimeofday(&evtime, NULL);

        evNr = 0;
        if (nowX != lastX) {
            sendev[evNr].time = evtime;
            sendev[evNr].type = EV_ABS;
            sendev[evNr].code = ABS_X;
            sendev[evNr++].value = lastX = nowX;
        }
        if (nowY != lastY) {
            sendev[evNr].time = evtime;
            sendev[evNr].type = EV_ABS;
            sendev[evNr].code = ABS_Y;
            sendev[evNr++].value = lastY = nowY;
        }
        if (lastLeft != nowLeft) {
            sendev[evNr].time = evtime;
            sendev[evNr].type = EV_KEY;
            sendev[evNr].code = BTN_LEFT;
            sendev[evNr++].value = lastLeft = nowLeft;
        }
        if (lastRight != nowRight) {
            sendev[evNr].time = evtime;
            sendev[evNr].type = EV_KEY;
            sendev[evNr].code = BTN_RIGHT;
            sendev[evNr++].value = lastRight = nowRight;
        }
        if (evNr)
            write(evfd, sendev, sizeof(struct input_event) * evNr);
    }

    return 0;
}
