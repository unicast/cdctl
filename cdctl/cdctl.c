/* 
	
vim:ts=4

$Id: cdctl.c,v 1.4 2001-07-31 09:07:27 thalakan Exp $ 

2.0 patches by Andy Thaller <andy_thaller@physik.tu-muenchen.de>
MSF patches by Jens Axboe <axboe@image.dk>

FIXME:

 - Implement the rest of the documented ioctl() calls
 - Implement figuring out how long the tracks are
 - Volume parsing is broken; only the left gets read for some reason

*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <ctype.h>

#include <sys/ioctl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <getopt.h>

#include <asm/errno.h>
#include <errno.h>
#include <err.h>

#include <signal.h>

#include <libintl.h>

#include <linux/cdrom.h>
#ifdef _LINUX_UCDROM_H
#include <linux/ucdrom.h> /* need this to have all the CDS_* stuff (andy)*/
#endif
#include "cdctl.h"


int main(int argc, char *argv[]) {

    int ret;
    int i;
	char *arg = "";
	int done = 0;
	int cdrom = -1;
	int option = 0; 

	static char options[] = "ab:cd:eghiklmno::p::rst:u:v::V";
	static struct option longoptions[] = {
		{ "pause",	    0,	NULL,	'a' },
		{ "speed",          1,  NULL,   'b' },
		{ "close",	    0,	NULL,	'c' },
		{ "disc",           1,  NULL,   'd' },
		{ "eject",	    0,	NULL,	'e' },
		{ "getstatus",      0,  NULL,   'g' },
		{ "help",	    0,	NULL,	'h' },
		{ "iso-header",     0,  NULL,   'i' },
		{ "iso-header-hex", 0,  NULL,   '2' },

#ifdef CDROM_GET_CAPABILITY
		{ "capabilities",   0,  NULL,   'k' },
#endif
		{ "list",           0,  NULL,   'l' },
		{ "mcn",            0,  NULL,   'm' },
#ifdef CDROM_LOCKDOOR
		{ "lockdoor",       2,  NULL,   'o' },
#endif
#ifdef CDROM_CHANGER_NSLOTS
		{ "numslots",       0,  NULL,   '1' },
#endif
#ifdef HAVE_DVD_IOCTLS
		{ "dvdinfo",        0,  NULL,   'n' },
#endif
		{ "play",	    2,	NULL,	'p' },
		{ "resume",	    0,	NULL,	'r' },
		{ "stop",	    0,	NULL,	's' },
		{ "tocentry",	    1,	NULL,	't' },
		{ "autoclose",      1,  NULL,   'u' },
		{ "volume", 	    2,  NULL,   'v' },
		{ "version",	    0,	NULL,	'V' },
	};

	static struct action actions[] = {
		{ "eject", cd_eject },
		{ "shut", cd_close },
		{ "open", cd_eject },
		{ "close", cd_close },
		{ NULL, NULL }
	};

/*
 * Begin master initalization 
 */
	
	signal(SIGINT, sig_handler);

	textdomain("cdctl");
	
	/* FIXME: the locale directory should be snarfed from autoconf */
	
	bindtextdomain("cdctl", "/usr/share/locale");

	while((ret = getopt_long(argc,argv,options,longoptions,NULL)) != EOF) {

		switch(ret) {
			case -1: 
				usage();
				return 1;
			default:
				if(done != 0) {
					warnx("You must give only one option.\n");
					usage();
					return 1;
				}
				option = ret;
				arg = optarg;

				done = 1;
				break;
		} 
	}
	
	optarg = arg;

/*
 * End master initialization, try to find the cdrom device.
 * In order of priority:
 *  1: as a command-line argument
 *  2: as an environment variable
 *  3: "/dev/cdrom"
 *   - if *that* fails, turn on DEBUG and start from 3 again
 *  4: "/dev/scd0"
 *  5: "/dev/sr0"
 */
    if(argv[optind] != NULL) {
		cdrom = open_cdrom(argv[optind]);
		if(cdrom == -1) {
			warn(_("can't open %s"), argv[optind]);
			return 1;
		}
	}
	if(getenv("CDROM") != NULL) {
		cdrom = open_cdrom(getenv("CDROM"));
		if(cdrom == -1) {
			warn(_("can't open %s"), getenv("CDROM"));
		}
	}
	if(cdrom == -1) {
		cdrom = open_cdrom("/dev/cdrom");
    }
    if(cdrom == -1) { 
		warnx(_("Um, I can't seem to open /dev/cdrom.  Trying harder..."));
		setenv("DEBUG", "1", 0);
		cdrom = open_cdrom("/dev/cdrom");
	}
	if(cdrom == -1) { /* SCSI cdroms */
		cdrom = open_cdrom("/dev/scd0");
	}
	if(cdrom == -1) {
		cdrom = open_cdrom("/dev/sr0");
	}

	if (cdrom == -1) {
			warnx(_("Hmm, I can't seem to open any cd or dvd device.  Try giving the\n"
			"name of a cdrom device (/dev/hdc, maybe?) to cdctl."));

			return 1;
	}

/* 
 * Various possibilities for symlinks to trigger eject or close.  
 * actions is an array of struct action, and action is a function
 * pointer to the action to be performed.
 */

        for(i = 0; actions[i].name != NULL; ++i) {
            if(strstr(argv[0], actions[i].name) != NULL) {
                (* actions[i].action)(cdrom); 
                return 0;
            }
        }
/* Ok, it's open or we're dead.  Proceed. */

	switch(option) {
		case 'V': 
			version();
			break;
		case 'h':
			usage();
			break;
		case 'b':
			do_select_speed(cdrom, optarg);
			break;
		case 'e':
			cd_eject(cdrom);
			break;
		case 'c':
			cd_close(cdrom);
			break;
		case 'd':
			do_select_disc(cdrom, optarg);
			break;
		case 'a':
			cd_pause(cdrom);
			break;
	    case 'i':  
			do_dump_header(cdrom, 0); /* 0 == no hexdump */
			break;
		case 'r':
			cd_resume(cdrom);
	        	break;
		case 'p':
			do_play(cdrom);
			break;
		case 's':
			cd_stop(cdrom);
			break;
		case 'k':
			do_print_capabilities(cdrom);
			break;
		case 'm':
			do_print_mcn(cdrom);
			break;
	        case 'n':
			do_print_dvdinfo(cdrom);
			break;
		case 'u':
			cd_set_autoeject(cdrom, atoi(optarg));
			break;
		case 'g':
			do_print_status(cdrom);
		   	break;
		case 'l':
			do_print_toc(cdrom);
			break;
#ifdef CDROM_LOCKDOOR
		case 'o':
			if(optarg != NULL) {
				cd_lockdoor(cdrom, atoi(optarg));
			} else {
				warnx(_("you must supply a numeric argument for the o option"));
			}
			break;
#endif
		case 't':
			if(optarg != NULL) {
				if(atoi(optarg) != 0) {
					do_print_tocentry(cdrom, atoi(optarg));
				} else {
					warnx(_("you must supply a numeric argument for the t option"));
				}
			} else {
				warnx(_("you must supply a numeric argument for the t option"));
			}
			break;
#ifdef CDROM_CHANGER_NSLOTS
		case '1':
			printf("%i", cd_get_changer_nslots(cdrom));
			break;
#endif
    	case '2':
			do_dump_header(cdrom, 1); /* 1 == hexdump too */
			break;

		case 'v':
			if(optarg == 0) {
				do_print_volume(cdrom);
			} else {
				do_set_volume(cdrom, optarg);
			}
			break;
		default:
			usage();
			break;
	}

	close(cdrom);

	return 0;
}


int do_select_disc(int cdrom, char *input) {

	if(optarg != NULL) {

/* 
 * code borrowed from Documentation/cdrom/ide-cd
 */

		if(cd_get_changer_nslots(cdrom) > 1) {
			cd_select_disc(cdrom, atoi(optarg));
		} else {
			warnx("your drive is not an ATAPI complaint jukebox.");
			exit(1);
		}
	} 
	return 1;
}
	

int do_print_mcn(int cdrom) {
	struct cdrom_mcn *ret;
	
	ret = cd_get_mcn(cdrom);
	if(ret == NULL) {
		return 0;
	} else if(ret->medium_catalog_number == NULL) {
		return 0;
	} else if(ret->medium_catalog_number[13] != 0) {
		warnx("Got wierd struct cdrom_mcn from the cdrom driver.  Notify the\n"
              "maintainers (cdctl --help) immediately¸ you've found a bug!\n");
		return 0;
	} else {
		if(getenv("DEBUG")) {
			printf("DEBUG: mcn[0] is %i", ret->medium_catalog_number[0]);
		}
		printf("%s\n", ret->medium_catalog_number);
	}
	return 1;
}


int do_print_capabilities(int cdrom) {
	
	int ret;

	ret = cd_get_capability(cdrom);

	if(getenv("DEBUG")) {
		printf("DEBUG: cd_get_capability returned %i", ret);
	}

	printf("Tray close             : %i\n", (ret & CDC_CLOSE_TRAY)?1:0);
	printf("Tray open              : %i\n", (ret & CDC_OPEN_TRAY)?1:0);
	printf("Can disable eject      : %i\n", (ret & CDC_LOCK)?1:0);
	printf("Selectable spin speed  : %i\n", (ret & CDC_SELECT_SPEED)?1:0);
	printf("Is a jukebox           : %i\n", (ret & CDC_SELECT_DISC)?1:0);
	if(ret & CDC_SELECT_DISC) 
	printf("Number of slots        : %i\n", cd_get_changer_nslots(cdrom));
	printf("Is multisession capable: %i\n", (ret & CDC_MULTI_SESSION)?1:0);
	printf("Can read the MCN (UPC) : %i\n", (ret & CDC_MCN)?1:0);
	printf("Can report media change: %i\n", (ret & CDC_MEDIA_CHANGED)?1:0);
	printf("Can play audio discs   : %i\n", (ret & CDC_PLAY_AUDIO)?1:0);
	printf("Can do a hard reset    : %i\n", (ret & CDC_RESET)?1:0);
	printf("Has non-standard ioctls: %i\n", (ret & CDC_IOCTLS)?1:0);
	printf("Can report drive status: %i\n", (ret & CDC_DRIVE_STATUS)?1:0);

#ifdef CDROM_CHANGER_NSLOTS
	if(cd_get_changer_nslots(cdrom) <= 1 && (ret & CDC_SELECT_DISC)) {
		printf("WARNING: your cdrom drive says that it is a jukebox, but\n"
               "only reports that it has one slot!\n");
	}
#endif

	return ret;
}


int do_print_tocentry(int cdrom, int track) {
		
	struct cdrom_tocentry *entry;
	
	entry = cd_get_tocentry(cdrom, track);
	if(entry == NULL) {
		return 0;
	}

	printf("*** Track info for cd track %i ***\n"
			"track start: %i:%i:%i\n"
			"This track is %s track.\n",
			entry->cdte_track,
			entry->cdte_addr.msf.minute, entry->cdte_addr.msf.second,
			entry->cdte_addr.msf.frame,
			entry->cdte_ctrl == 4 ? "a data" : "an audio");
	return 1;
}


int do_print_volume(int cdrom) {
	
	struct cdrom_volctrl *volume;

	volume = cd_read_volume(cdrom);

	if(volume != NULL) {
		printf("L : %i\n", volume->channel0);
		printf("R : %i\n", volume->channel1);
		printf("LR: %i\n", volume->channel2);
		printf("RR: %i\n", volume->channel3);
		return 1;
	} else {
		return -1;
	}
}


int do_set_volume(int cdrom, char *input) {
	
	struct cdrom_volctrl *volume;
	const char delim[] = ":- ";
	char *one = (char *)NULL;	/* initialized here because unconditionally */
	char *two = (char *)NULL;	/* freed below, even if never allocated */
	char *tmp;


	volume = cd_read_volume(cdrom);
	
	if(input != NULL) {
		tmp = strtok(input, delim);
		if(tmp != NULL) {
		    one = malloc(sizeof(char) * strlen(tmp) + 1);
			if(one == NULL) {
			    err(1, "can't allocate memory!");
			}
			memset(one, 0, sizeof(char) * strlen(tmp) + 1);
			strcpy(one, tmp);
		}
	}
	
	/* 
     * Ok, we're going to assume that the user doesn't want the volume
	 * changed if we get a weird argument.  If *I* screwed up and
	 * entered a weird argument, I know I wouldn't want my sound system to
	 * suddenly cut out or jump to maximum volume!
     */

	if(getenv("DEBUG")) {
		printf("DEBUG: do_set_volume(): one is %i\n", atoi(one));
	}

	if(one != NULL) {
		volume->channel0 = atoi(one);
	}
		
	tmp = strtok(NULL, delim);
	if(tmp != NULL) {
		two = malloc(sizeof(char) * strlen(tmp) + 1);
		if(two == NULL) {
			err(1, "can't allocate memory!");
		}
		memset(two, 0, sizeof(char) * strlen(tmp) + 1);
		strcpy(two, tmp);
	}
	
	if(getenv("DEBUG")) {
		printf("DEBUG: do_set_volume(): two is %i\n", atoi(two));
	}

	/* If only one volume given, set it on both channels. */
	if(two != NULL) {
		volume->channel1 = atoi(two);
	} else {
		if (one != NULL) {
			volume->channel1 = atoi(one);
		}
	}
	
	free(one);
	free(two);
	
	return cd_set_volume(cdrom, volume);
}


int do_print_toc(int cdrom) {

	struct cdrom_tochdr *header;
	struct cdrom_tocentry *entry;
	int i;
	
	header = cd_get_audio_tracks(cdrom);
	
	if(header == 0) {
		return 0;
	} else {
		for(i = header->cdth_trk0; i <= header->cdth_trk1; ++i) {
			entry = cd_get_tocentry(cdrom, i);
			if(entry == 0) {
				continue;
			}
			printf("Track %2i: %s, starts at %.2i:%.2i:%.2i\n", 
					i, 
					entry->cdte_ctrl == 4 ? " data" : "audio",
					entry->cdte_addr.msf.minute,
					entry->cdte_addr.msf.second,
					entry->cdte_addr.msf.frame);
					
		}
	}
	return 1;
}


int do_select_speed(int cdrom, char *speed) {
		
	int ret;
		
	if(speed == NULL) {
		warnx("you must supply an argument to --speed.\n");
		exit(1);
	}
	
	ret = atoi(speed);
	
	if(ret <= 0) {
		warnx("you must supply a numeric argument greater than 0 to --speed.\n");
		exit(1);
	}
	
	return cd_select_speed(cdrom, ret);
}


int do_print_status(int cdrom) {
	
	switch(cd_get_status(cdrom)) {
	
		case CDS_NO_INFO:
			printf("There is no CD/DVD in the drive (no info).\n");
			break;
		case CDS_NO_DISC:
			printf("There is no CD/DVD in the drive (no disc).\n");
			break;
		case CDS_TRAY_OPEN:
			printf("The CD/DVD drive tray is open.\n");
			break;
		case CDS_DRIVE_NOT_READY:
			printf("The CD/DVD drive is not ready.  Try again.\n");
			break;
		case CDS_DISC_OK:
			printf("The CD/DVD drive is OK now.\n");
			break;
		case CDS_AUDIO:
			printf("There is an audio CD/DVD in the drive.\n");
			break;
		case CDS_DATA_1:
			printf("There is a type 1 data CD/DVD in the drive.\n");
			break;
		case CDS_DATA_2:
			printf("There is a type 2 data CD/DVD in the drive.\n");
			break;
		case CDS_XA_2_1:
			printf("There is a type 1 multisession CD/DVD in the drive.\n");
			break;
		case CDS_XA_2_2:
			printf("There is a type 2 multisession CD/DVD in the drive.\n");
			break;
#ifdef CDS_MIXED          /* (andy) */
		case CDS_MIXED:
			printf("There is a mixed-mode CD/DVD in the drive.\n");
			break;
#else                     /* (andy) */
#warning The executable will not have support for mixed-mode CD/DVDs
#endif                    /* (andy) */
	}
	
	return 0;
}


int do_play(int cdrom) {

	struct cdrom_tochdr *tracks;
	
	tracks = do_get_audio_tracks(cdrom);
	
	if(tracks == NULL) {

		if(getenv("DEBUG")) {
			printf("DEBUG: cd_get_status() got %i", cd_get_status(cdrom));
		}

		if(cd_get_status(cdrom) == CDS_NO_DISC) {
			warnx("Um, there's no disc in the drive.");
		} else if(cd_get_status(cdrom) == CDS_NO_INFO) {
			warnx("Um, there's no disc in the drive.");
		} else if(cd_get_status(cdrom) == CDS_AUDIO || cd_get_status(cdrom) == CDS_MIXED) {
			warnx("Um, this is not an audio CD.");
		} else {
			warnx("NULL cdrom_tochdr structure passed to do_play()");
		}
		exit(1);
	}
	
	return cd_play(cdrom, tracks->cdth_trk0, tracks->cdth_trk1);
}		


int do_print_dvdinfo(int cdrom) {
	int i;
	int ret;
	dvd_struct dvdinfo;

	memset(&dvdinfo, 0, sizeof(dvd_struct));
	ret = ioctl(cdrom, DVD_READ_STRUCT, &dvdinfo);
	if(ret == -1) {
		err(1, "couldn't read dvd info");
	}
	printf("Total layers: %i\n", dvdinfo.physical.layer[0].nlayers + 1);

	memset(&dvdinfo, 0, sizeof(dvd_struct));
	dvdinfo.physical.layer_num = 0;
 
	ret = ioctl(cdrom, DVD_READ_STRUCT, &dvdinfo);
	if(ret == -1) {
		err(1, "couldn't read dvd info");
	}

	for(i = 0; i <= dvdinfo.physical.layer[0].nlayers; ++i) {
		memset(&dvdinfo, 0, sizeof(dvd_struct));
		dvdinfo.type = DVD_STRUCT_PHYSICAL;
		dvdinfo.physical.layer_num = i;
 
		ret = ioctl(cdrom, DVD_READ_STRUCT, &dvdinfo);
		if(ret == -1) {
			err(1, "couldn't read dvd info");
		}

		printf("*** Layer %i ***\n\n", i);
		printf("book_version  : %i\n", 
		       dvdinfo.physical.layer[i].book_version);
		
		printf("book_type     : %i ", dvdinfo.physical.layer[i].book_type);
		switch(dvdinfo.physical.layer[i].book_type) {
		case 0:
			printf("(DVD-ROM)\n");
			break;
		case 1: 
			printf("(DVD_RAM)\n");
			break;
		case 2: 
			printf("(DVD_R)\n");
			break;
		case 3: 
			printf("(DVD-RW)\n");
			break;
		case 9: 
			printf("(DVD+RW)\n");
			break;
		default: 
			printf("(reserved)\n");
			break;
		}

		printf("min_rate      : %i ",
		       dvdinfo.physical.layer[i].min_rate);

		switch(dvdinfo.physical.layer[i].min_rate) {
		case 0:
			printf("(2.52Mbps)\n");
			break;
		case 1:
			printf("(5.04Mbps)\n");
			break;
		case 2:
			printf("(10.08Mbps)\n");
			break;
		case 15:
			printf("(none specified)\n");
			break;
		default:
			printf("(reserved)\n");
			break;
		}

		printf("disc_size     : %i ",
		       dvdinfo.physical.layer[i].disc_size);
		switch(dvdinfo.physical.layer[i].disc_size) {
		case 0: 
			printf("(120mm disc)\n");
			break;
		case 1: 
			printf("(80mm disc)\n");
			break;
		default:
			printf("(reserved)\n");
			break;
		}

		printf("layer_type    : %i ", 
		       dvdinfo.physical.layer[i].layer_type);
		switch(dvdinfo.physical.layer[i].layer_type) {
		case 0:
			printf("(embossed area)\n");
			break;
		case 1:
			printf("(recordable area)\n");
			break;
		case 2:
			printf("(re-writable area)\n");
			break;
		case 3:
			printf("(reserved)\n");
			break;
		}

		printf("track_path    : %i ",
		       dvdinfo.physical.layer[i].track_path);
		switch(dvdinfo.physical.layer[i].track_path) {
		case 0: 
			printf("(PTP dual layer or single layer)\n");
			break;
		case 1: 
			printf("(OTP)\n");
			break;
		default:
			printf("(reserved)\n");
			break;
		}

		printf("nlayers       : %i ",
		       dvdinfo.physical.layer[i].nlayers);
		switch(dvdinfo.physical.layer[i].nlayers) {
		case 0:
			printf("(one layer)\n");
			break;
		case 1:
			printf("(two layers)\n");
			break;
		default:
			printf("(reserved)\n");
			break;
		}

		printf("track_density : %i ",
		       dvdinfo.physical.layer[i].track_density);
		switch(dvdinfo.physical.layer[i].track_density) {
		case 0:
			printf("(0.74 micrometers/track)\n");
			break;
		case 1:
			printf("(0.80 micrometers/track)\n");
			break;
		case 2:
			printf("(0.615 micrometers/track)\n");
			break;
		default:
			printf("(reserved)\n");
			break;
		}

		printf("linear_density: %i ",
		       dvdinfo.physical.layer[i].track_density);
		switch(dvdinfo.physical.layer[i].track_density) {
		case 0:
			printf("(0.267 micrometers/bit)\n");
			break;
		case 1:
			printf("(0.293 micrometers/bit)\n");
			break;
		case 2:
			printf("(0.409~0.435 micrometers/bit)\n");
			break;
		case 4:
			printf("(0.280~0.291 micrometers/bit)\n");
			break;
		case 8:
			printf("(0.353 micrometers/bit)\n");
			break;
		default:
			printf("(reserved)\n");
			break;

		}

		printf("bca           : %i %s\n",
		       dvdinfo.physical.layer[i].bca,
		       dvdinfo.physical.layer[i].bca ? 
		       "(bca present)" : "(no bca)"
		       );

		printf("start_sector  : %i (0x%x)\n", 
		       dvdinfo.physical.layer[i].start_sector, 
		       dvdinfo.physical.layer[i].start_sector);

		printf("end_sector    : %i (0x%x)\n",
		       dvdinfo.physical.layer[i].end_sector,
		       dvdinfo.physical.layer[i].end_sector);

		printf("end_sector_l0 : %i (0x%x)\n",
		       dvdinfo.physical.layer[i].end_sector_l0,
		       dvdinfo.physical.layer[i].end_sector_l0);
	}
	return 1;
}


struct cdrom_tochdr *do_get_audio_tracks(int cdrom) {
	
	struct cdrom_tochdr *tracks;
	char *arg = optarg;
	char start[8], finish[8];
	int i = 0, j = 0, parse_finish = 0, ret = 0;
	
	memset(&start, 0, 8);
	memset(&finish, 0, 8);
	
	if(arg == NULL) {
		tracks = cd_get_audio_tracks(cdrom);    /* default to all tracks */
		return tracks;
	}
	
	if(! isdigit(arg[0]) && arg[0] != ':' && arg[0] != '-') {
		warnx("The play option requires a numeric argument.");
		exit(1);
	} 

	for(i = 0; i < strlen(arg); ++i) {
		
		if(arg[i] == 0) break;		/* No argument given, get all tracks */
		
		if(parse_finish == 0) {
			if(isdigit(arg[i])) {   /* Ok, it's a number */
				start[i] = arg[i]; 
				if(arg[i+1] == 0) {  /* Only the start track was given */
					parse_finish = 1;
					break;
				}
				continue;            /* keep going... */
			} else if(arg[i] == '-' || arg[i] == ':') { 
				parse_finish = 1; /* go ahead and parse the finish track */
				continue;
			} else if(arg[i] == 0) {
				/* no argument at all, skip the parsing */
				break;
			} else {
				warnx("invalid argument to play... say cdctl -p"
					  "START-FINISH or START:FINISH.\n");
				exit(1);
			}

		} else { /* done with start track, parse finish track */
				
			if(isdigit(arg[i])) {
				finish[j] = arg[i];
				j++;
				parse_finish = 2;
				continue;
			} else if(arg[i] == 0) {
				parse_finish = 2;
				break;
			} else if(arg[i] == ':') {
					
			} else {
				warnx("invalid argument to play... say cdctl -p"
					  "START-FINISH or START:FINISH.\n");
				exit(1);
			}
			
		}
	}
	
	tracks = malloc(sizeof(struct cdrom_tochdr));
	
	if(tracks == NULL) {
	warnx("can't allocate memory!");
		exit(1);
	}

	if(parse_finish == 0) {		/* No tracks given, default to all */
		ret = ioctl(cdrom, CDROMREADTOCHDR, tracks);
		if(ret == -1) {
			warn("couldn't read cd/dvd table of contents");
			exit(1);
		}
	}
	
	tracks = cd_get_audio_tracks(cdrom);

	if(parse_finish >= 1) {		/* Start track given */
		if(atoi(start) < tracks->cdth_trk0) {
			if(getenv("DEBUG")) {
				warnx("can't start playing at track %i, the first track is %i.",
					atoi(start), tracks->cdth_trk0);
			}
		}
		
		if(atoi(start) > tracks->cdth_trk1) {
				warnx("can't play track %i, there's only %i track(s)!",
					atoi(start), tracks->cdth_trk1);
			exit(1);
		}

		if(atoi(start) != 0) {
			tracks->cdth_trk0 = atoi(start);
		}
	}
	
	if(parse_finish >= 2) {		/* ... and finish track given */
		if(atoi(finish) > tracks->cdth_trk1) {
			warnx("can't play through track %i, the last track is %i.",
				atoi(finish), tracks->cdth_trk1);
			exit(1);
		}
		
		if(atoi(finish) != 0) {                 /* it's valid, right? */
			tracks->cdth_trk1 = atoi(finish);
		}
	}
	
	if(tracks->cdth_trk0 > tracks->cdth_trk1) { /* tracks are backwards */
		i = tracks->cdth_trk0;
		tracks->cdth_trk0 = tracks->cdth_trk1;  /* switch tracks */
		tracks->cdth_trk1 = i;
		if(getenv("DEBUG")) {
			warnx("um, you put the first track last and last track first.");
		}
	}

	return tracks;
}


int is_empty(int cdrom) {

	 switch(cd_get_status(cdrom)) {
	 
		case CDS_NO_INFO:
   		case CDS_NO_DISC:
		case CDS_TRAY_OPEN:
		case CDS_DRIVE_NOT_READY:
		case CDS_DISC_OK:
    		 return 0;
			 break;
		case CDS_AUDIO:
   			 return 1;
			 break;
		case CDS_DATA_1:
   		case CDS_DATA_2:
   		case CDS_XA_2_1:
		case CDS_XA_2_2:
		     return 0;
			 break;
        default:
		     return 0;
             break;
	 }
}


int is_audio_cd(int cdrom) {

	 switch(cd_get_status(cdrom)) {
	 
		case CDS_NO_INFO:
   		case CDS_NO_DISC:
		case CDS_TRAY_OPEN:
		case CDS_DRIVE_NOT_READY:
		case CDS_DISC_OK:
    		 return 0;
			 break;
		case CDS_AUDIO:
   			 return 1;
			 break;
		case CDS_DATA_1:
   		case CDS_DATA_2:
   		case CDS_XA_2_1:
		case CDS_XA_2_2:
		     return 0;
			 break;
	    default:
             return 0;
             break;
	 }
}


int is_data_cd(int cdrom) {

	 switch(cd_get_status(cdrom)) {
	 
		case CDS_NO_INFO:
   		case CDS_NO_DISC:
		case CDS_TRAY_OPEN:
		case CDS_DRIVE_NOT_READY:
		case CDS_DISC_OK:
    		 return 0;
			 break;
		case CDS_AUDIO:
   			 return 0;
			 break;
		case CDS_DATA_1:
   		case CDS_DATA_2:
   		case CDS_XA_2_1:
		case CDS_XA_2_2:
		     return 1;
			 break;
	    default:
             return 0;
             break;
	 }
}


int open_cdrom(char *cdrom_dev_file) {
	
	int cdrom;

	cdrom = open(cdrom_dev_file, O_RDONLY | O_NONBLOCK);
        if(cdrom == -1) {
            if(getenv("DEBUG")) {
                warn("can't open \"%s\"", cdrom_dev_file);
            }
		return -1;
	} else {
		return cdrom;
	}
}


void sig_handler(int signal) {
	if(getenv("DEBUG")) {
		printf("Why, we just got signal number %i!\n", signal);
	}
	if(signal == SIGINT) {
		fflush(stdout);
		fflush(stderr);
		exit(1);
	}    
}
