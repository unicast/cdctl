/* 
 * ioctl.c
 *
 * generalized ioctl() calls for cdctl.c
 */

#include <stdio.h>			/* standard libs */
#include <stdlib.h>

#include <sys/ioctl.h>		/* ioctl() and friends */

#include <malloc.h>		/* malloc() */

#include <err.h>		/* warn(), err(), and friends */
#include <asm/errno.h>
#include <errno.h>

#include <asm/errno.h>		/* WTF, there's no check to see if EOPNOTSUPP is
                            checked before it's included in cdrom.h... */
#include <linux/cdrom.h>	/* cdrom ioctl defs and stuff */
#ifdef _LINUX_UCDROM_H
#include <linux/ucdrom.h> /* need ths to have all the CDS_* stuff (andy)*/
#endif

#include "cdctl.h"


int cd_eject(int cdrom) {
	int ret;
	
	ret = ioctl(cdrom, CDROMEJECT);

#ifdef EDRIVE_CANT_DO_THIS	
	if(ret == -EDRIVE_CANT_DO_THIS) {
		warnx("Your cd/dvd drive can't eject its tray or caddy.\n");
		return 0;
	}
#endif
	
	if(ret == -1) {
		warn("can't eject cd/dvd device");
		return -1;
	}
	return 0;
}


struct cdrom_mcn *cd_get_mcn(int cdrom) {
	int ret;
	struct cdrom_mcn *mcn;
	
	mcn = malloc(sizeof(struct cdrom_mcn));
	if(mcn == NULL) {
		warn("cannot allocate memory!");
		exit(1);
	}
	
	ret = ioctl(cdrom, CDROM_GET_MCN, mcn);
	if(ret == -1) {
		warn("can't read the mcn from the drive");
		return NULL;
	} else {
		return mcn;
	}
	return mcn;
}


int cd_stop(int cdrom) {
	int ret;
	
	ret = ioctl(cdrom, CDROMSTOP);
	
	if(ret == -1) {
		warn("can't stop the cd/dvd drive");
		exit(1);
	} else {
		return 1;
	}
}


int cd_close(int cdrom) {
	int ret;

	ret = ioctl(cdrom, CDROMCLOSETRAY);

#ifdef EDRIVE_CANT_DO_THIS	
	if(ret == -EDRIVE_CANT_DO_THIS || ret == -EIO) {
		warnx("Your cd/dvd drive can't close its tray, or does not have "
		      "a tray.\n");
		return 0;
	}
#endif	

	if(ret == -1) {
		warn("can't close cd/dvd drive");
		return 0;
	}

	return 1;
}


int cd_pause(int cdrom) {
	int ret;
	
	ret = ioctl(cdrom, CDROMPAUSE);
	
	if(ret == -1) {
		warn("can't pause cd/dvd drive");
		return 0;
	}
	
	return 1;
}


int cd_resume(int cdrom) {
	int ret;
	
	ret = ioctl(cdrom, CDROMRESUME);
	
	if(ret == -1) {
		if(errno == EIO) {
			warnx("Try pausing a CD before you try this option.");
			exit(1);
		}
		warn("can't resume playing cd/dvd drive");
		return 0;
	}
	
	return 1;
}

/*
struct cdrom_tocentry
{
        u_char  cdte_track; 			INPUT: track to get
        u_char  cdte_adr        :4;     ?  What's this used for?
        u_char  cdte_ctrl       :4;     Whether this is audio or data
        u_char  cdte_format;            LBA or MSF addressing, LBA not valid
        union cdrom_addr cdte_addr;     Address of track start
        u_char  cdte_datamode;          Not used except by sbpcd, and even
										then looks like it's a bogus field
};            
*/
struct cdrom_tocentry *cd_get_tocentry(int cdrom, int track) {

	int ret;
	struct cdrom_tocentry *entry;
	
	entry = malloc(sizeof(struct cdrom_tocentry));
	
	if(entry == NULL) {
		warn("can't allocate memory!");
		exit(1);
	}
	
	entry->cdte_track = track;
	entry->cdte_format = CDROM_MSF;
		
	ret = ioctl(cdrom, CDROMREADTOCENTRY, entry);
	
	if(ret == -1) {
		warn("can't read table of contents entry for track %i", track);
		return NULL;
	} else {
		return entry;
	}
	

	return entry;
}


struct cdrom_tochdr *cd_get_audio_tracks(int cdrom) {

	int i;
	int ret;
	struct cdrom_tochdr *tracks;
	struct cdrom_tocentry *entry;

	tracks = malloc(sizeof(struct cdrom_tochdr));
	
	if(tracks == NULL) {
		warn("couldn't allocate memory!");
		exit(1);
	}
	
	ret = ioctl(cdrom, CDROMREADTOCHDR, tracks);

	if(ret == -1) {
		warn("can't read cdrom table of contents");
		if(cd_get_status(cdrom) == CDS_NO_INFO) {
			warnx("Um, I don't think there's a cd in the drive.");
		} else if(cd_get_status(cdrom) == CDS_NO_DISC) {
			warnx("Um, I don't think there's a cd in the drive.");
		}
		exit(1);
	}

	if(tracks == NULL) {
		warnx("cd_get_audio_tracks() got a NULL struct cdrom_tochdr!");
		exit(1);
	}

	for(i = 1; i <= tracks->cdth_trk1; ++i) {
		entry = cd_get_tocentry(cdrom, i);
		if(entry == NULL) {
			warnx("do_get_audio_tracks() got a NULL struct cdrom_tocentry!");
			exit(1);
		}
		if(entry->cdte_ctrl == 4) {

/*
 * Ok, found the first data track, so stick a cap on the end of the struct and
 * return it.
 * FIXME: what about mixed-mode CDs?
 */
			tracks->cdth_trk1 = i;
			return tracks;
		}
	}
			

	return tracks;
}


int cd_play(int cdrom, int start, int finish) {
	int ret;
	struct cdrom_msf *msf;
	struct cdrom_tocentry *entry;

	msf = malloc(sizeof(struct cdrom_msf));
	
/*
 * No driver actually uses the index fields, but we're going to
 * initialize them anyway for good luck.
 */

	msf = malloc(sizeof(struct cdrom_msf));

	/* get start and finish track msf address */
	entry = cd_get_tocentry(cdrom, start);
	msf->cdmsf_min0 = entry->cdte_addr.msf.minute;
	msf->cdmsf_sec0 = entry->cdte_addr.msf.second;
    msf->cdmsf_frame0 = entry->cdte_addr.msf.frame;

    entry = cd_get_tocentry(cdrom, finish);
    msf->cdmsf_min1 = entry->cdte_addr.msf.minute;
    msf->cdmsf_sec1 = entry->cdte_addr.msf.second;
    msf->cdmsf_frame1 = entry->cdte_addr.msf.frame;

	if(getenv("DEBUG")) {
		printf("DEBUG: playing tracks %i through %i\n", start, finish);
	}
	
	ret = ioctl(cdrom, CDROMPLAYMSF, msf);
	
	if(ret == -1) {
		warn("can't play cd/dvd device");
		return 0;
	}
	
	free(entry);
	free(msf);
	
	return 1;
}


int cd_select_disc(int cdrom, int disc) {
	int ret;
	
	ret = ioctl(cdrom, CDROM_SELECT_DISC, disc);
	if(ret == -1) {
		warn("can't change disc");
		return -1;
	}

	return 1;
}


int cd_lockdoor(int cdrom, int arg) {
	
	int ret = 0;

#ifdef CDROM_LOCKDOOR
	ret = ioctl(cdrom, CDROM_LOCKDOOR, arg);

	if(ret == -1) {
		warn("can't lock the cdrom door!");
		exit(1);
	}
#else
#warning Your kernel does not have support for door locking from software.
#warning You need at least kernel version 2.2.4, cdrom driver version 2.53.
#endif
	return ret;
}


int cd_get_capability(int cdrom) {

	
	int ret = 0;
	int cap = 0;

#ifdef CDROM_GET_CAPABILITY

	cap = ioctl(cdrom, CDROM_GET_CAPABILITY, &cap);
	if(cap == -1) {
		warn("can't get drive capabilities!");
		exit(1);
	}
#else
#warning Your kernel does not support capability readback.  
#warning You need at least kernel version 2.2.4, cdrom driver version 2.53
#endif

	ret++;

	return cap;
}


int cd_get_changer_nslots(int cdrom) {
	int ret = '?';

#ifdef CDROM_CHANGER_NSLOTS
	ret = ioctl(cdrom, CDROM_CHANGER_NSLOTS);

	if(ret == -1) {
		warn("can't get number of changer slots");
		return -1;
	}
#endif

	return ret;
}


int cd_select_speed(int cdrom, int speed) {
	int ret;

	ret = ioctl(cdrom, CDROM_SELECT_SPEED, speed);

	if(ret == -1) {
		warn("can't set drive speed!");
		return -1;
	}
	
	if(ret == -ENOSYS) {
		warn("the drive does not support speed selection!");
		return -1;
	}
	
	return ret;
}


int cd_set_volume(int cdrom, struct cdrom_volctrl *volume) {
	int ret;

	if(volume == NULL) {
		warnx("NULL struct cdrom_volctrl passed to cd_set_volume()");
		return -1;
	}

	ret = ioctl(cdrom, CDROMVOLCTRL, volume);
	
	if(ret == -1) {
		warn("can't set cdrom volume!");
		return -1;
	}
	return 1;
}
	

struct cdrom_volctrl *cd_read_volume(int cdrom) {
	int ret;
	struct cdrom_volctrl *volume;

	volume = malloc(sizeof(struct cdrom_volctrl));
	
	if(volume == NULL) {
		err(1, "can't allocate memory!");
	}

	ret = ioctl(cdrom, CDROMVOLREAD, volume);

	if(ret == -1) {
		warn("can't read volume");
		return NULL;
	}

	return volume;
}


int cd_get_status(int cdrom) {
	int ret;
	
	ret = ioctl(cdrom, CDROM_DISC_STATUS);
	if(ret == -1) {
		warn("can't get cd status");
		return -1;
	}
	return ret;
}


int cd_set_autoeject(int cdrom, int arg) {
	int ret;

	ret = ioctl(cdrom, CDROMEJECT_SW, arg);
	if(ret == -1) {
		warn("can't set auto-eject!");
		return -1;
	}
	if(getenv("DEBUG")) {
		printf("autoeject successful!  ioctl() returned %i.", ret);
	}
	return ret;
}

/*
 * vim:ts=4
 */
