/*
 * <srFolder/sr_aux.c>
 *
 * This file contains the ambient function. It fill the Ambient set of the bash before launch it
 *
 * Note, the copyright+license information is at end of file.
 */



#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include <sys/capability.h>
#include <cap-ng.h>
#include <sys/prctl.h>

#define TOK_START               "= +"
#define TOK_FLOAT               ","

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


int main (int argc, char *argv[])
{
	char *user = argv[2];
	char *s1 = "/home/";
	char *s2 = "/.bashrc";
	char *result = malloc(strlen(s1) + strlen(s2) + strlen(user) + 1);
	strcpy(result,s1);
	strcat(result,user);
	strcat(result,s2);
	
	FILE* bash = NULL;
	bash = fopen(result,"a");
	if (bash == NULL) {
		perror("Open fail : .bashrc\n");
		exit(EXIT_FAILURE);
	}
	fprintf(bash,"PS1=\"\\u-%s@\\h:\\W \"",argv[1]);
	fclose(bash);
	
	while (wait(NULL) == -1 && errno == EINTR);
	
	add_ambient();
	
	if (fork()) {
		while (wait(NULL) == -1 && errno == EINTR);
		char *newargv[] = { NULL };
		char *newenviron[] = { NULL };

		newargv[0] = "scriptBash.sh";
		newargv[1] = user;
		newargv[2] = NULL;
		execve(newargv[0],newargv,newenviron);
		perror("execve");   /* execve() ne retourne qu'en cas d'erreur */
		exit(EXIT_FAILURE);
		
		exit(EXIT_SUCCESS);
	}
	
	printf("bash launch\n");
	/* Here we launch the bash with the capabilities in P, E, I and A. */
	char *newargv[] = { NULL };
	char *newenviron[] = { NULL };

	newargv[0] = "/bin/bash";
	newargv[1] = NULL;
	execve(newargv[0],newargv,newenviron);
	perror("execve");   /* execve() ne retourne qu'en cas d'erreur */
	exit(EXIT_FAILURE);
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
