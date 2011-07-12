/* 
 * vim:ts=4
 */

#include <stdio.h>            /* standard libraries */

#include <malloc.h>           /* malloc() and friends */

#include <asm/errno.h>        /* human-readable error numbers */
#include <errno.h>
#include <err.h>
#include "cdctl.h"

struct rt *last_rt = NULL;

int rt_init(void) {
	last_rt = NULL;
	return 1;
}


int add_rt(void *thingie) {
	struct rt *new_rt;
	
	new_rt = malloc(sizeof(struct rt));
	
	if(new_rt == NULL) {
		warnx("can't allocate memory!");
		errno = -ENOMEM;
	    return -1;
	} else {
		if(thingie != NULL) {
				
/* FIXME: we should probably rewrite sig_handler so it dies screaming the 
   right thing when it knows it segfaulted because of this */

			new_rt->thingie = thingie;      /* assign the thingie */
		} else {
			errno = -EFAULT;
			return -1;
		}
		new_rt->used = 1;               /* mark the new one used */
		new_rt->prev_rt = last_rt;      /* link the left end */
		new_rt->next_rt = NULL;         /* link the right end and terminate */
		last_rt = new_rt;               /* move the end-of-chain marker up */
		return 1;
	}
}


int remove_rt(struct rt *red_tape) {

	if(red_tape->used == 0) {                   /* Flag says it's not used */
		if(red_tape->thingie != NULL) {         /* But... it is used! */
				
				/* Oh, no. */
				
			warnx("oh, crap!  memory allocation is all f*cked up!\n");
			errno = -EFAULT;
			return -1;
		} else {
			_link(red_tape->prev_rt, red_tape->next_rt);
			free(red_tape);
		}
	} else {                                    /* Flag says it's used */
		free(red_tape->thingie);                /* free the thingie */
		_link(red_tape->prev_rt, red_tape->next_rt);  
		return 1;
	}
	return 1;
}


int _link(struct rt *one, struct rt *two) {
	if(two != NULL) {
		one->next_rt = two;
	}
	if(one != NULL) {
		two->prev_rt= one;
	}
	return 1;
}
