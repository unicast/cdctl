#define VERSION "cdctl 0.14-devel"

/* 
 * rt == red tape, async signal handling in C, arrrgggggghhhhhhhh
 *
 * struct rt is a element in a linked list.   There is a global pointer
 * to the last one, global struct rt *last_rt;.  
 */

struct rt {      
	int used;             /* malloc()d or not? */
	void *thingie;        /* pointer to thing */
	struct rt *prev_rt;   /* pointer to previous struct rt */
	struct rt *next_rt;   /* pointer to next struct rt */
};

/* 
 * NLS stuff
 */

#define _(STRING) gettext(STRING)

/*
 * Generic program stuff
 */

extern void                    usage(void);
extern void                    version(void);
extern void                    sig_handler(int signal);

/*
 * Direct ioctl(2) wrappers
 */

extern int                     cd_stop(int cdrom);
extern int                     cd_eject(int cdrom);
extern int                     cd_close(int cdrom);
extern int                     cd_pause(int cdrom);
extern int                     cd_resume(int cdrom);
extern struct cdrom_mcn *      cd_get_mcn(int cdrom);
extern int                     cd_get_status(int cdrom);
extern struct cdrom_volctrl *  cd_read_volume(int cdrom);
extern int                     cd_get_capability(int cdrom);
extern struct cdrom_tochdr *   cd_get_audio_tracks(int cdrom);
extern int                     cd_lockdoor(int cdrom, int arg);
extern int                     cd_get_changer_nslots(int cdrom);
extern int                     cd_select_disc(int cdrom, int disc);
extern int                     cd_set_autoeject(int cdrom, int arg);
extern int                     cd_select_speed(int cdrom, int speed);
extern struct cdrom_tocentry * cd_get_tocentry(int cdrom, int track);
extern int                     cd_play(int cdrom, int start, int finish);
int                            cd_set_volume(int cdrom, struct cdrom_volctrl *volume);

/*
 * Helper functions to wrap around the cd_* calls for arg parsing, etc.
 */

extern char *                  strtrim(char *b);
extern int                     do_play(int cdrom);
extern int                     do_print_toc(int cdrom);
extern int                     do_print_mcn(int cdrom);
extern int                     do_dump_header(int cdrom, int hex);
extern int                     do_print_status(int cdrom);
extern int                     do_print_volume(int cdrom);
extern char *                  do_parse_iso_date(char *date);
extern int                     do_print_capabilities(int cdrom);
extern int                     do_set_volume(int cdrom, char *input);
extern int                     do_select_disc(int cdrom, char *input);
extern int                     do_select_speed(int cdrom, char *speed);
extern int                     do_print_tocentry(int cdrom, int track);
extern void *                  do_get_block(int fd, int offset, int len);

/*
 * my dinky API
 */

extern int is_empty(int cdrom);
extern int is_data_cd(int cdrom);
extern int is_audio_cd(int cdrom);

/* 
 * other stuff
 */

extern struct cdrom_tochdr *   do_get_audio_tracks(int cdrom);
extern int                     open_cdrom(char *cdrom_dev_file);

/* 
 * red tape library 
 */

extern int    _link(struct rt *one, struct rt *two);
extern int    remove_rt(struct rt *red_tape);
extern int    add_rt(void *thingie);
extern int    rt_init(void);

