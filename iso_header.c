#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include <ctype.h>

#include <malloc.h>

#include <time.h>

#include <err.h>

#include "cdctl.h"

/* 	$Id: iso_header.c,v 1.2 2000-04-11 09:19:58 thalakan Exp $	 */

/*
 * Types of values we find in the ISO 9660 headers for struct iso_header
 * 
 * DO NOT DO NOT REFER TO THESE VALUES DIRECTLY; THEY ARE REDEFINED
 * OFTEN AND ARE NOT GUARANTEED TO BE STATIC.
 *
 * FIXME: M-% and changes these to 9660_text or something; ISO is a 
 * bad prefix
 */

#define ISO_TEXT    1
#define ISO_DATE    2
#define ISO_INT     3
#define ISO_VOLID   4
#define ISO_VOLDESC 5

struct iso_header_entry {
	 char * name; /* name of field */
	 int off;     /* offset of field */
	 int len;     /* length of field */
	 int type;    /* type of field (see above) */
};


int do_dump_header(int cdrom, int hex) {

	 int i;                              /* i is for int or index */
	 int col;							 /* c is for column index */
	 char * s;                           /* s is for string */ 
	 struct iso_header_entry * current;  /* current is for current entry */
	 struct iso_header_entry iso_header[] = {
		  { "First Volume Descriptor", 0x8000, 1,    ISO_VOLDESC },
		  { "Volume ID              ", 0x8001, 6,    ISO_VOLID   },
		  { "System ID              ", 0x8008, 0x1f, ISO_TEXT    },
		  { "Application ID         ", 0x823e, 0x7f, ISO_TEXT    },
		  { "Volume Set ID          ", 0x827f, 0x7f, ISO_TEXT    },
          { "Preparer ID            ", 0x81be, 0x7f, ISO_TEXT    },
		  { NULL, 0, 0, 0 },
     };


/* 
 * sanity check 1: is this a data cd?
 * Some drives are dumb and can't tell the difference.
 */

#ifndef MY_DRIVE_IS_A_RETARD

	 if(!is_data_cd(cdrom)) {
		  errx(1, "Can't dump ISO header; the cdrom says this is not a data "
                  "CD.");
	 }

#endif /* MY_DRIVE_IS_A_RETARD */

	 current = &iso_header[0];

	 while(current->name != NULL) {

		  s = (char *) do_get_block(cdrom, current->off, current->len);

/*
 * If we're not in hex mode, trim the strings to cut down on verbosity
 */

		  if(!hex) {
			   strtrim(s);
		  }

		  switch(current->type) {
		  case ISO_TEXT:
			   printf("%s: \"%s\"\n", current->name, s);
			   col = 0;

/* 
 * FIXME: this hex dump is ugly
 */

			   if(hex) {
					for(i = 0; *(s + i) != 0; ++i) {
						 if((col + 3) > 80) {
							  printf("\n");
							  col = 0;
						 }
						 col += printf("%x ", *(s + i));
					}
					printf("\n\n");
			   }
			   break;
	  
		  case ISO_DATE:
			   printf("%s: \"%s\"\n", current->name, s);
			   break;

		  case ISO_INT:
			   printf("%s: \"%s\"\n", current->name, s);
			   break;

		  case ISO_VOLID:

/*
 * sanity check 2: is the volume identifier correct?  Should be "CD001".
 * FIXME: ECMA 168 says that this could be CDW02 or NSR02 as well.
 */

/* 
 * If we're in hex mode, the user probably doesn't care whether it's valid or
 * not; they just want the raw data.
 */

			   if(!hex){
  			   if(strcmp(s, "CD001") != 0) {
					warnx("Got wrong volume id \"%s\"; this might not be "
						  "an ISO9660 CD...", s);
			   }
			   }
			   printf("%s: \"%s\"\n", current->name, s);
			   break;
			   
		  case ISO_VOLDESC:
			   switch((char) *s) {
			   case 0:
					printf("%s: \"0\" (Boot Record)\n", current->name);
					break;
			   case 1:
					printf("%s: \"1\" (Primary Volume Descriptor)\n", 
						   current->name);
					break;
			   case -1:
					printf("%s: \"%i\" (Bad descriptor data!)\n", 
						   current->name, (int) *s);
					break;
			   default:
					printf("%s: \"%i\" (Unrecognized descriptor type)\n", 
						   current->name, (int) *s);
					break;
			   }
		  }
		  
		  current = current + 1;
		  free(s);
	 }

	 return 1;
}


void *do_get_block(int fd, int o, int len) {
	
	 int ret;
	 ssize_t size;
	 void *b;         /* b is for block */

	 b = malloc(len + 1);

	 if(b == NULL) {
		  warnx("can't allocate memory!");
		  exit(1);
	 }

/* 
 * zero the buffer + 1 for NULL
 */

	 memset(b, 0, len + 1);

/* 
 * seek to the given offset
 */
	 ret = lseek(fd, o, SEEK_SET);

	 if(ret == -1) {
		  warn("can't seek cdrom!");
		  exit(1);
	 }

	 size = read(fd, b, len);

	 return b;
}


char *strtrim(char * s) {

/*
 * WARNING WARNING modifies argument!  Just cause it looks like a library 
 * function doesn't mean it is!
 */

	 int empty = 1;
	 int size = 0;
	 int end = 0;
	 int i;

	 if(s == NULL) {
		  errx(1, "NULL pointer passed to strtrim()");
	 }

	 size = strlen(s);

/* 
 * Pass 1: check for all spaces or nulls
 */
	
	for(i = 0; i < size; ++i) {
		if(*(s + i) == ' ' || *(s + i) == 0) {
			empty = 1;
			continue;
		} else {
			empty = 0;
			break;
		}
	}
	
	if(empty) {

/*
 * s is empty.  Say so.
 */

        memset(s, 0, size);
		strcpy(s, "(empty)");
		return s;
	}

/*
 * Pass 2: remove any trailing spaces by reading from the end of 
 * s going backwards
 */
	
	for(i = size; i > 0; --i) {
		if(isgraph(*(s + i))) {
			end = i + 1;
			break;
		}
	}
	
	if(end != 0) {
		*(s + end) = 0;
	}
	
	return s;
}	

