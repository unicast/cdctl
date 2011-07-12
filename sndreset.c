#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/soundcard.h>
#include <err.h>
#include <malloc.h>
#include <signal.h>
#include <fcntl.h>


int main(void) {
	int dsp, vol, ret;
	
	dsp = open("/dev/dsp", O_WRONLY);
	if(dsp == -1) {
		warn("can't open /dev/dsp");
		exit(1);
	}
	
	ret = ioctl(dsp, SOUND_MIXER_READ_CD, &vol);
	if(ret == -1) {
		warn("can't execute READ_CD ioctl");
		exit(1);
	}
	
	ret = ioctl(dsp, SOUND_MIXER_WRITE_CD, &vol);
	if(ret == -1) {
		warn("can't execute WRITE_CD ioctl");
		exit(1);
	}
	
	if(getenv("DEBUG")) {
		printf("READ_CD returned %i\n", vol);
	}
	
	return 0;
}
