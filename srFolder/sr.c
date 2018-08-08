/*
 * <srFolder/sr.c>
 *
 * This file contains my main program, the read_capabilities and set_capabilities functions.
 * It's here I set the Permitted, Effective and Inheritable capabilities for the new bash.
 *
 * Note, the copyright+license information is at end of file.
 */



#include <stdio.h>
#include <stdlib.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <getopt.h>

#include <sys/capability.h>
#include <cap-ng.h>
#include <sys/prctl.h>

#define USER_CAP_FILE_ROLE	"/etc/security/capabilityRole.conf"
#define CAP_FILE_BUFFER_SIZE    4096
#define CAP_FILE_DELIMITERS     ": \t\n"
#define CAP_COMBINED_FORMAT     "%s all-i %s+i"
#define CAP_DROP_ALL            "%s all-i"

#define TOK_FLOAT               ","
#define TOK_BAR			":"
#define TOK_SPACE		" "

#define TOK_START               "= +"

#define MAX_LEN			512


/* Conv struct for the PAM authentification */
const struct pam_conv conv = {
	misc_conv,
	NULL
};

/* struct from pam_cap */
struct pam_cap_s {
    int debug;
    char *user;
    char *role;
};



/* Set the capabilities in the Ambient set */
static void add_ambient(void)
{
	char *proc_epcaps;
	char *capToCap_ng;
	cap_t cap_s;

	cap_s = cap_get_proc();
	proc_epcaps = cap_to_text(cap_s, NULL);
	char *str1, *str2, *token, *subtoken;
	int i, stringToCap;

	capToCap_ng = malloc(sizeof(char*));
	for (str1 = proc_epcaps; ;str1 = NULL) {
		token = strtok(str1,TOK_START);
		if (token == NULL || !strcmp(token,"e") || !strcmp(token,"i") || !strcmp(token,"p")
		|| !strcmp(token,"ei") || !strcmp(token,"ep") || !strcmp(token,"ip") || !strcmp(token,"eip"))
			break;
			
		for (i = 0,str2 = token; ;i++,str2 = NULL) {
			subtoken = strtok(str2,TOK_FLOAT);
			if (subtoken == NULL)
				break;

			/* We need this loop because the "#defined" to attribute an integer to a capabilility in cap-ng
			is without CAP_ */
			for (int j = 4;subtoken[j-1] != '\0';j++) {
				capToCap_ng[j-4] = subtoken[j];
			}

			stringToCap = capng_name_to_capability(capToCap_ng);

			prctl(PR_CAP_AMBIENT,PR_CAP_AMBIENT_RAISE,stringToCap,0,0);
		}
	}

	free(capToCap_ng);
	cap_free(cap_s);
}

/* Read the role's capabilities */
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
	int none = 0;
	while ((line = fgets(buffer, CAP_FILE_BUFFER_SIZE, cap_file))) {
		const char *cap_role;
		char *lineBis;
		char *lineTer;
		char *saveptr1, *saveptr2;

		cap_role = strtok_r(line, CAP_FILE_DELIMITERS, &saveptr1);

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
				if (!cpt)
					cap_string = line;
					
				if (cpt == 1) {
					lineBis = line;
					lineTer = strtok_r(lineBis,TOK_FLOAT,&saveptr2);
					
					if (!strcmp(lineTer,user)) {
						//printf("User found\n");
						found_one = 1;
						break;
					}
					while (lineTer = strtok_r(NULL,TOK_FLOAT,&saveptr2)) {
						if (!strcmp(lineTer,user)) {
							//printf("User found\n");
							found_one = 1;
							break;
						}
					}
				}
				if (cpt == 2) {
					lineBis = line;
					lineTer = strtok_r(lineBis,TOK_FLOAT,&saveptr2);
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
					}
						
					for (int j = 0;j<i;j++) {
						if (!strcmp(list[j],lineTer)) {
							//printf("User is in this group : %s\n",lineTer);
							found_one = 1;
							break;
						}
						if (!strcmp(lineTer,"none")) {
							none = 1;
							break;
						}
					}
						
					if (found_one || none)
						break;
							
					
					while (lineTer = strtok_r(NULL, TOK_FLOAT, &saveptr2)) {
						i = 0;
						for (str = listString;i<2;i++,str = NULL) {
							token = strtok(str,TOK_BAR);
						}

						list = NULL;
						size = 1;
						for (i = 0,str = token;;i++,str = NULL) {
							list = realloc(list, sizeof(void*) * (size));
							subtoken = strtok(str,TOK_SPACE);
							
							if (subtoken == NULL)
								break;
							
							list[i] = subtoken;
							size++;
						}
						
						int testGroup = 0;
						for (int j = 0;j<i;j++) {
							if (!strcmp(list[j],lineTer)) {
								//printf("User is in this group : %s\n",lineTer);
								found_one = 1;
								break;
							}
						}
						
						if (found_one)
							break;
						
					}
					free(listString);
					free(list);
					pclose(fGroup);
				}
				
				cpt++;
				D(("user is not [%s] - skipping", line));
			}
		}

		cap_role = NULL;
		line = NULL;

		if (found_one) {
			D(("user is allowed to use this role"));
			break;
		}
		if (none) {
			D(("user is not allowed to use this role and group are none"));
			break;
		} 
	}

	fclose(cap_file);

	if (!found_one || none) {
		memset(buffer, 0, CAP_FILE_BUFFER_SIZE);
		cap_string = NULL;
	}

	return cap_string;
}

/* Put the role's capabilities from the struct to the kernel */
int set_capabilities(struct pam_cap_s *cs)
{
	cap_t caps;
	int ok = 0;
	char *conf_icaps;
	
	caps = cap_get_proc();
	if (caps == NULL)
		perror("No capabilities");
		
	if (cap_clear_flag(caps,CAP_INHERITABLE) == -1)
		perror("Problem when clearing Inheritable\n");
		
	conf_icaps = read_capabilities_for_role(cs->user, cs->role);
	if (conf_icaps == NULL) {
		printf("No capabilities are allowed\n");
		exit(EXIT_SUCCESS);
	}
		
	cap_value_t *cap_list = malloc(strlen(conf_icaps) * sizeof(cap_value_t));
	if (cap_list == NULL)
		perror("unable to combine capabilities into one string or set in a list - no memory\n");
	
	/* Extract caps in string format from conf_icaps to cap_value_t format */
	int i;
	char *str1, *token;
	int cpt = 0;
	for (i = 0,str1 = conf_icaps; ;i++,str1 = NULL) {
		token = strtok(str1,TOK_FLOAT);
		//printf("%s\n",token);
		if (token == NULL)
			break;
		cap_from_name(token, &cap_list[i]);
		cpt++;
	}
	
	if (cap_set_flag(caps, CAP_INHERITABLE, cpt, cap_list, CAP_SET) == -1)
		perror("Problem with Inheritable\n");

	if (cap_set_proc(caps) == -1) {
		perror("Capabilities were not set\n");
	} else{
		ok = 1;
	}

	cap_free(caps);
	free(cap_list);
	return ok;
}


/* Set the setfcap capability in the inheritable set. Used when we want to use setcap for sr_aux */
int set_setfcap(void)
{
	cap_t caps;
	int ok = 0;
	
	caps = cap_get_proc();
	if (caps == NULL)
		perror("No capabilities");
		
	cap_value_t *cap_list = malloc(sizeof(cap_value_t));
	if (cap_list == NULL)
		perror("unable to combine capabilities into one string or set in a list - no memory\n");
	
	/* Extract caps in string format from conf_icaps to cap_value_t format */
	cap_from_name("cap_setfcap", &cap_list[0]);
	
	if (cap_set_flag(caps, CAP_INHERITABLE, 1, cap_list, CAP_SET) == -1)
		perror("Problem with Inheritable\n");

	if (cap_set_proc(caps) == -1) {
		perror("Capabilities were not set\n");
	} else{
		ok = 1;
	}

	cap_free(caps);
	free(cap_list);
	return ok;
}

/* Set the setpcap capability in the effective set and clear the inheritable set. Used to set the role in Inheritable */
int set_setpcap(void)
{
	cap_t caps;
	int ok = 0;
	
	caps = cap_get_proc();
	if (caps == NULL)
		perror("No capabilities");
		
	if (cap_clear_flag(caps,CAP_INHERITABLE) == -1)
		perror("Problem when clearing Inheritable\n");
	if (cap_clear_flag(caps,CAP_EFFECTIVE) == -1)
		perror("Problem when clearing Effective\n");
		
	cap_value_t *cap_list = malloc(sizeof(cap_value_t));
	if (cap_list == NULL)
		perror("unable to combine capabilities into one string or set in a list - no memory\n");
	
	/* Extract caps in string format from conf_icaps to cap_value_t format */
	cap_from_name("cap_setpcap", &cap_list[0]);
	
	if (cap_set_flag(caps, CAP_EFFECTIVE, 1, cap_list, CAP_SET) == -1)
		perror("Problem with Inheritable\n");

	if (cap_set_proc(caps) == -1) {
		perror("Capabilities were not set\n");
	} else{
		ok = 1;
	}

	cap_free(caps);
	free(cap_list);
	return ok;
}

/* Set the dac_override capability in the effective set. Used before the authentification section */
int set_dac_override(void)
{
	cap_t caps;
	int ok = 0;
	
	caps = cap_get_proc();
	if (caps == NULL)
		perror("No capabilities");
		
	cap_value_t *cap_list = malloc(sizeof(cap_value_t));
	if (cap_list == NULL)
		perror("unable to combine capabilities into one string or set in a list - no memory\n");
	
	/* Extract caps in string format from conf_icaps to cap_value_t format */
	cap_from_name("cap_dac_override", &cap_list[0]);
	
	if (cap_set_flag(caps, CAP_EFFECTIVE, 1, cap_list, CAP_SET) == -1)
		perror("Problem with Inheritable\n");

	if (cap_set_proc(caps) == -1) {
		perror("Capabilities were not set\n");
	} else{
		ok = 1;
	}

	cap_free(caps);
	free(cap_list);
	return ok;
}


int get_setuid_setgid()
{
	cap_t caps;
	cap_flag_value_t value_setuid;
	cap_flag_value_t value_setgid;
	
	caps = cap_get_proc();
	if (caps == NULL)
		perror("No capabilities");
		
	cap_get_flag(caps, CAP_SETUID, CAP_EFFECTIVE, &value_setuid);
	cap_get_flag(caps, CAP_SETGID, CAP_EFFECTIVE, &value_setgid);
	
	if ((value_setuid == CAP_SET) || (value_setgid == CAP_SET)) {
		cap_free(caps);
		return 1;
	} else{
		cap_free(caps);
		return 0;
	}
}



/* We need a fork for the execve setcap. Without fork, we will lose controle on this processus after the execve */
void fork_setcap(char* user, char* role, int noroot, char *command)
{	
	struct pam_cap_s pcs;
	
	/* sr_aux will be used to fill the ambient set of the process and launch the bash.
	But, if more than 1 user want to use Switch Role at the same time, it's a problem.
	For this reason, the program make a copy of sr_aux and it will work on it.
	The copy has an unique name (sr_aux_userName_role). If user is root, the new file will go to /home/
	else, it will go to /home/username/ */
	
	char *srAuxUnique;
	if (!strcmp(user,"root")) {
		char *srAux = "sr_aux_";
		char *home = "/home/";
		srAuxUnique = malloc(strlen(home) + strlen(srAux) + strlen(user) + strlen(role) + strlen("_") + 1);
		strcpy(srAuxUnique,home);
		strcat(srAuxUnique,srAux);
		strcat(srAuxUnique,user);
		strcat(srAuxUnique,"_");
		strcat(srAuxUnique,role);
	} else{
		char *srAux = "sr_aux_";
		char *home = "/home/";
		srAuxUnique = malloc(strlen(home) + strlen("/") + strlen(srAux) + strlen(user) + strlen(role) + strlen("_") + 1);
		strcpy(srAuxUnique,home);
		strcat(srAuxUnique,user);
		strcat(srAuxUnique,"/");
		strcat(srAuxUnique,srAux);
		strcat(srAuxUnique,user);
		strcat(srAuxUnique,"_");
		strcat(srAuxUnique,role);
	}
	
	if (fork()) {
		/* wait as long as any child is there */
		while (wait(NULL) == -1 && errno == EINTR);

		if (fork()) {
			while(wait(NULL) == -1 && errno == EINTR);
			
			char *rm = "rm ";
			char *silent = " 2> /dev/null";
			char *rmSr = malloc(strlen(rm) + strlen(srAuxUnique) + strlen(silent) + 1);
			strcpy(rmSr,rm);
			strcat(rmSr,srAuxUnique);
			strcat(rmSr,silent);
			
			//printf("Remove sr_aux_bis\n");
			system(rmSr);
			
			free(rmSr);
			free(srAuxUnique);
			exit(EXIT_SUCCESS);
		}
		pcs.user = user;
		pcs.role = role;
		set_capabilities(&pcs);
		
		//printf("sr_aux_bis launch\n");
		/* Here we launch the aux process with the capabilities in P, E and I.
		It will add the Ambient capabilities and launch the bash */
		char *newargv[] = { NULL };
		char *newenviron[] = { NULL };
		
		newargv[0] = srAuxUnique;
		char* rootArg;
		if (noroot)
			rootArg = "noroot";
		else
			rootArg = "root";
		newargv[1] = rootArg;;
		newargv[2] = read_capabilities_for_role(user,role);
		newargv[3] = command;
		newargv[4] = NULL;
		execve(newargv[0],newargv,newenviron);
		perror("execve");   /* execve() ne retourne qu'en cas d'erreur */
		exit(EXIT_FAILURE);	
	}

	/* Exec the setcap command to set the capabilities in the sr_aux binary */
	char *newargv[] = { NULL };
	char *newenviron[] = { NULL };

	char *s1 = read_capabilities_for_role(user,role);
	if (s1 == NULL) {
		if (noroot)
			printf("You need to specifie a role after the -noroot argument\n");
		else
			printf("Role doesn't exist or it's not allowed for this user\n");
		exit(EXIT_FAILURE);
	}
	char *s2 = "+p";
	char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
	strcpy(result, s1);
	strcat(result, s2);
	
	char *cp = "cp /usr/bin/sr_aux ";
	char *cpSr = malloc(strlen(cp) + strlen(srAuxUnique) + 1);
	strcpy(cpSr,cp);
	strcat(cpSr,srAuxUnique);
	
	system(cpSr);
	free(cpSr);

	newargv[0] = "/sbin/setcap";
	newargv[1] = result;
	newargv[2] = srAuxUnique;
	newargv[3] = NULL;

	execve(newargv[0], newargv, newenviron);
	perror("execve");   /* execve() ne retourne qu'en cas d'erreur */
	
	exit(EXIT_SUCCESS);
}


int main(int argc, char *argv[])
{
	pam_handle_t* pamh = NULL;
	int retval;
	char* user = NULL;
	int noroot = 0;
	char *command = NULL;
	char *role = NULL;
	
	static const struct option longopts[] = {
		{"command", required_argument, NULL, 'c'},
		{"user", required_argument, NULL, 'u'},
		{"role", required_argument, NULL, 'r'},
		{"noroot", no_argument, 0, 'n'},
		{"help", no_argument, 0, 'h'},
		{NULL, 0, NULL, 0}
	};
	
	int optc;
	while ((optc = getopt_long(argc, argv, "c:u:r:nh", longopts, NULL)) != -1) {
		switch (optc) {
		case 'c':
			command = optarg;
			break;
		
		case 'u':
			user = optarg;
			break;
			
		case 'r':
			role = optarg;
			break;
			
		case 'n':
			noroot = 1;
			break;
			
		case 'h':
			printf("Usage : sr -r role [-u user] [-c commande] [-n]\n");
			exit(EXIT_FAILURE);
			break;
			
		default:
			exit(EXIT_FAILURE);
		}
	}
	
	if (role == NULL) {
		printf("Usage : sr -r role [-u user] [-c commande] [-n]\n");
		exit(EXIT_FAILURE);
	}
	
	if (user != NULL) { //If user want to launch a bash with an other user
		
		if (get_setuid_setgid())  {
			char *echo = "echo `grep ";
			char *cut = " /etc/passwd | cut -d: -f3`";
			char *takeId = malloc(strlen(echo) + strlen(cut) + strlen(user) + 1);
			strcpy(takeId,echo);
			strcat(takeId,user);
			strcat(takeId,cut);
			
			FILE *fId = popen(takeId, "r"); //Take only the uid of the specified user
			char *id = malloc(sizeof(fId));
			fgets(id,MAX_LEN,fId);
			
			free(takeId);
			pclose(fId);
			prctl(PR_SET_KEEPCAPS,1,0,0,0);
			setuid(atoi(id));
		} else {
			printf("Can't switch user, cap_setuid and cap_setgid are not set\n");
			exit(EXIT_FAILURE);
		}
		
	} else{
		/* username is used for authentification and role access control */
		user = getenv("USER");
		
		//printf("User : %s\n",user);
	
		retval = pam_start("check_user", user, &conv, &pamh);

		retval = pam_setcred(pamh, 0);
		// Are the credentials correct?
		if (retval == PAM_SUCCESS) {
			set_dac_override();
			retval = pam_authenticate(pamh, 0);
		}

		// Can the account be used at this time?
		if (retval == PAM_SUCCESS) {
			//printf("Account is valid.\n");
			retval = pam_acct_mgmt(pamh, 0);
		} else{
			printf("Authentification failed.\n");
			exit(EXIT_FAILURE);
		}
		
		// Did everything work?
		if (retval == PAM_SUCCESS) {
			//printf("Authenticated\n");
		} else {
			printf("Not Authenticated\n");
		}

		// close PAM (end session)
		if (pam_end(pamh, retval) != PAM_SUCCESS) {
			pamh = NULL;
			printf("check_user: failed to release authenticator\n");
			exit(1);
		}
	}

	
	set_setpcap();

	set_setfcap();
	add_ambient();

	/* child will set capabilities and launch sr_aux */
	fork_setcap(user,role,noroot,command);
	
	//free(user);

	exit(EXIT_SUCCESS);
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
