#include <stdio.h>		/* standard libs */

#ifndef _LINUX_TYPES_H
#include <sys/types.h>
#endif

/* 
 * We're going to test against EOPNOTSUPP instead of _I386_ERRNO_H because we
 * don't want to be a i386 bigot, even though _H is the convention.
 */

#ifndef EOPNOTSUPP
#include <asm/errno.h>
#endif

/*
 * cdrom.h is ALL F*CKED UP!!!  Your cdrom.h probably assume that the
 * above two headers are already included, which is sometimes not the
 * case.  It took me hours to figure out what the problem was.  Grr.  
 * Jens Axboe <axboe@image.dk> has been notified, and the problem is 
 * fixed in recent kernel headers.
 */

#include <linux/cdrom.h>
#include "cdctl.h"

/*
 * The GNU coding standards say that you're supposed to give only the program
 * name and version, but I want the CVS Id as well.  Linus says we should
 * burn the coding standards anyway.
 */

static char vcid[] = "$Id: version.c,v 1.1 2000-03-27 01:06:54 thalakan Exp $";

void version(void) {
	printf("%s\n", VERSION);
	printf("%s\n", vcid);
}

	
/*
 * I really wish there was a command like perl's qw// to deal with
 * these situations.
 */

void usage(void) {
	fprintf(stderr, "Usage: cdctl [OPTION]\n"
"Controls the cdrom drive.\n\n"
"-a     --pause         pAuse the cdrom currently playing\n"
"-bS    --speed=S       Ask the drive to spin the disc at speed S\n"
"-c     --close         Close the cd tray if possible\n"
"-dD    --disc=D        select Disc D for a cd-changer\n"
"-e     --eject         Eject the cd, cd tray, or cd cartridge\n"
"-g     --getstatus     Get and print type/status of the cd in the drive\n"
"-h     --help          print this Help message\n"
"-i     --iso-header    print interesting Iso 9660 header data\n"
#ifdef CDROM_GET_CAPABILITY
"-k     --capabilities  Print out all the things your drive can do\n"
#endif
"-l     --list          List the entire table of contents\n"
"-m     --mcn           print MCN (or UPC) of the cd in the drive\n"
#ifdef CDROM_LOCKDOOR
"-oA    --lockdoor=A    lOck or unlock the eject button\n"
#endif
"-pT    --play=T        Play track T\n"
"-r     --resume        Resume playing\n"
"-s     --stop          Stop playing\n"
"-tT    --tocentry=T    print the Table of contents entry for track T\n"
"-uA    --automount=A   set aUtomount bits to A\n"
"-vV    --volume=V      set Volume to V\n"
"-V     --version       print Version\n\n"
"Send bug reports to Jason Spence <thalakan@technologist.com>\n");

}
