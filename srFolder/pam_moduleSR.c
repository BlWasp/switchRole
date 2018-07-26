/*
 * <srFolder/pam_moduleSR.c>
 *
 * This is my PAM module for credential and user authentification.
 *
 * Note, the copyright+license information is at end of file.
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include <security/_pam_macros.h>

#include <sys/capability.h>
#include <linux/capability.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include <security/pam_modules.h>
#include <security/_pam_macros.h>

#define USER_CAP_FILE_ROLE	"/etc/security/capabilityRole.conf"
#define CAP_FILE_BUFFER_SIZE    4096
#define CAP_FILE_DELIMITERS     " \t\n"
#define TOK_FLOAT               ","
#define TOK_BAR			":"
#define TOK_SPACE		" "

#define MAX_LEN			512

/* struct from pam_cap */
struct pam_cap_s {
    int debug;
    char *user;
    const char *userC;
    char *role;
    const char *conf_filename;
};


/* Read capabilities from user with the role wanted */
static char* read_capabilities_for_role(char *user, char *role)
{
	char *cap_string = NULL;
	char buffer[CAP_FILE_BUFFER_SIZE], *line;
	FILE *cap_file;

	cap_file = fopen(USER_CAP_FILE_ROLE, "r");
	if (cap_file == NULL) {
		D(("failed to open capability file"));
		return NULL;
	}

	int found_one = 0;
	while ((line = fgets(buffer, CAP_FILE_BUFFER_SIZE, cap_file))) {
		const char *cap_role;
		char *lineBis;
		char *lineTer;
		char *saveptr1, *saveptr2;

		cap_role = strtok_r(line, CAP_FILE_DELIMITERS, &saveptr1);
		//printf("cap_text : %s\n",cap_text);

		if (cap_role == NULL) {
		    D(("empty line"));
		    continue;
		}
		if (*cap_role == '#') {
		    D(("comment line"));
		    continue;
		}
		
		if (!strcmp(cap_role,role)) {
			int cpt = 0;
			while ((line = strtok_r(NULL, CAP_FILE_DELIMITERS, &saveptr1)) && !found_one) {
				//printf("Line1 : %s\n",line);
				if (!cpt)
					cap_string = line;
				//printf("Cap : %s\n",cap_string);
				if (cpt == 1) {
					lineBis = line;
					lineTer = strtok_r(lineBis,TOK_FLOAT,&saveptr2);
					
					//printf("Line2 : %s\n",line);
					//printf("lineTer1 : %s\n",lineTer);
					if (!strcmp(lineTer,user) || !strcmp(lineTer,"none")) {
						printf("User found\n");
						//printf("Cap : %s\n",cap_string);
						found_one = 1;
						break;
					}
					while (lineTer = strtok_r(NULL,TOK_FLOAT,&saveptr2)) {
						//printf("lineTer2 : %s\n",lineTer);
						if (!strcmp(lineTer,user) || !strcmp(lineTer,"none")) {
							printf("User found\n");
							found_one = 1;
							break;
						}
					}
				}
				//printf("%s\n",cap_string);
				if (cpt == 2) {
					lineBis = line;
					lineTer = strtok_r(lineBis,TOK_FLOAT,&saveptr2);
					//printf("Line3 : %s\n",line);
					//printf("lineTer3 : %s\n",lineTer);
					FILE *fGroup = popen("groups $USER","r");
					char *listString = malloc(sizeof(fGroup));
					fgets(listString,MAX_LEN,fGroup);

					char *str, *token;
					int i = 0;
					for (str = listString;i<2;i++,str = NULL) {
						token = strtok(str,TOK_BAR);
					}

					char **list = NULL;
					int size = 1;
					char *subtoken;
					for (i = 0,str = token;;i++,str = NULL) {
						list = realloc(list, sizeof(void*) * (size));
						subtoken = strtok(str,TOK_SPACE);
							
						if (subtoken == NULL)
							break;
		
						list[i] = subtoken;
						size++;
						//printf("%s\n",list[i]);
					}
						
					for (int j = 0;j<i;j++) {
						if (!strcmp(list[j],lineTer) || !strcmp(lineTer,"none")) {
							printf("User is in this group : %s\n",lineTer);
							found_one = 1;
							break;
						}
					}
						
					if (found_one)
						break;
							
					
					while (lineTer = strtok_r(NULL, TOK_FLOAT, &saveptr2)) {
						FILE *fGroup = popen("groups $USER","r");
						char *listString = malloc(sizeof(fGroup));
						fgets(listString,MAX_LEN,fGroup);
						
						char *str, *token;
						int i = 0;
						for (str = listString;i<2;i++,str = NULL) {
							token = strtok(str,TOK_BAR);
						}

						char **list = NULL;
						int size = 1;
						char *subtoken;
						for (i = 0,str = token;;i++,str = NULL) {
							list = realloc(list, sizeof(void*) * (size));
							subtoken = strtok(str,TOK_SPACE);
							
							if (subtoken == NULL)
								break;
							
							list[i] = subtoken;
							size++;
							//printf("%s\n",list[i]);
						}
						
						int testGroup = 0;
						for (int j = 0;j<i;j++) {
							if (!strcmp(list[j],lineTer) || !strcmp(lineTer,"none")) {
								printf("User is in this group : %s\n",lineTer);
								found_one = 1;
								break;
							}
						}
						
						if (found_one)
							break;
						
					}
				}
				
				cpt++;
				D(("user is not [%s] - skipping", line));
			}
		}

		cap_role = NULL;
		line = NULL;
		//printf("Cap3 : %s\n",cap_string);

		if (found_one) {
		    D(("user is allow to use this role"));
		    break;
		}
	}

	fclose(cap_file);

	if (!found_one) {
		memset(buffer, 0, CAP_FILE_BUFFER_SIZE);
		cap_string = NULL;
	}

	return cap_string;
}

static void parse_args(int argc, const char **argv, struct pam_cap_s *pcs)
{
    int ctrl=0;

    /* step through arguments */
    for (ctrl=0; argc-- > 0; ++argv) {

	if (!strcmp(*argv, "debug")) {
	    pcs->debug = 1;
	} else if (!memcmp(*argv, "config=", 7)) {
	    pcs->conf_filename = 7 + *argv;
	} else {
	    printf("probleme\n");
	    //_pam_log(LOG_ERR, "unknown option; %s", *argv);
	}

    }
}


/* expected hook */
PAM_EXTERN int pam_sm_setcred( pam_handle_t *pamh, int flags, int argc, const char **argv ) {
	printf("Setcred\n");
	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
	printf("Acct mgmt\n");
	return PAM_SUCCESS;
}

/* First test for new authentification in my module. Come from pam_cap, with modifications */
PAM_EXTERN int pam_sm_authenticate( pam_handle_t *pamh, int flags,int argc, const char **argv )
{
    int retval;
    struct pam_cap_s pcs;
    char *conf_icaps;

    memset(&pcs, 0, sizeof(pcs));

    parse_args(argc, argv, &pcs);

    retval = pam_get_user(pamh, &pcs.userC, NULL);

    if (retval == PAM_CONV_AGAIN) {
	D(("user conversation is not available yet"));
	memset(&pcs, 0, sizeof(pcs));
	return PAM_INCOMPLETE;
    }

    if (retval != PAM_SUCCESS) {
	D(("pam_get_user failed: %s", pam_strerror(pamh, retval)));
	memset(&pcs, 0, sizeof(pcs));
	return PAM_AUTH_ERR;
    }

    conf_icaps =
	read_capabilities_for_role(pcs.user, pcs.role);

    memset(&pcs, 0, sizeof(pcs));

    if (conf_icaps) {
	D(("it appears that there are capabilities for this user [%s]",
	   conf_icaps));

	/* We could also store this as a pam_[gs]et_data item for use
	   by the setcred call to follow. As it is, there is a small
	   race associated with a redundant read. Oh well, if you
	   care, send me a patch.. */

	_pam_overwrite(conf_icaps);
	_pam_drop(conf_icaps);

	return PAM_SUCCESS;

    } else {

	D(("there are no capabilities restrictions on this user"));
	return PAM_IGNORE;

    }
}


/* ... adapted from the pam_cap.c file created by Andrew G. Morgan
 *
 * Copyright Guillaume Daumas <guillaume.daumas@univ-tlse3.fr>, 2018
 * Copyright Ahmad Samer Wazan <ahmad-samer.wazan@irit.fr>, 2018
 * Copyright (c) Andrew G. Morgan <morgan@linux.kernel.org>, 1996-8
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, and the entire permission notice in its entirety,
 *    including the disclaimer of warranties.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * ALTERNATIVELY, this product may be distributed under the terms of
 * the GNU Public License, in which case the provisions of the GPL are
 * required INSTEAD OF the above restrictions.  (This clause is
 * necessary due to a potential bad interaction between the GPL and
 * the restrictions contained in a BSD-style copyright.)
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.  */
