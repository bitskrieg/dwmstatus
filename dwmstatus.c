#define _BSD_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <X11/Xlib.h>

char *tzindia = "Asia/Kolkata";

const char *BATT_NOW = "/sys/class/power_supply/BAT0/energy_now";
const char *BATT_FULL = "/sys/class/power_supply/BAT0/energy_full";
const char *BATT_STATUS = "/sys/class/power_supply/BAT0/status";

static Display *dpy;

char *
smprintf(char *fmt, ...)
{
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len);
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

void
settz(char *tzname)
{
	setenv("TZ", tzname, 1);
}

char *
mktimes(char *fmt, char *tzname)
{
	char buf[129];
	time_t tim;
	struct tm *timtm;

	memset(buf, 0, sizeof(buf));
	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL) {
		perror("localtime");
		exit(1);
	}

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		fprintf(stderr, "strftime == 0\n");
		exit(1);
	}

	return smprintf("%s", buf);
}


char *
getbattery(){
    long lnum1, lnum2 = 0;
    char *status = malloc(sizeof(char)*12);
    char s = '?';
    FILE *fp = NULL;
    if ((fp = fopen(BATT_NOW, "r"))) {
        fscanf(fp, "%ld\n", &lnum1);
        fclose(fp);
        fp = fopen(BATT_FULL, "r");
        fscanf(fp, "%ld\n", &lnum2);
        fclose(fp);
        fp = fopen(BATT_STATUS, "r");
        fscanf(fp, "%s\n", status);
        fclose(fp);
        if (strcmp(status,"Charging") == 0)
            s = '+';
        if (strcmp(status,"Discharging") == 0)
            s = '-';
        if (strcmp(status,"Full") == 0)
            s = '=';
        return smprintf("%c%ld%%", s,(lnum1/(lnum2/100)));
    }
    else return smprintf("");
}

void
setstatus(char *str)
{
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}


int
main(void)
{
	char *status;
	char *tmindia;
        char *batt;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "dwmstatus: cannot open display.\n");
		return 1;
	}

	for (;;sleep(1)) {
		tmindia = mktimes("%a %d,%H:%M", tzindia);
                batt = getbattery();

		status = smprintf("%s | %s", batt, tmindia);
		setstatus(status);
		free(tmindia);
                free(batt);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}

