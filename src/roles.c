/*
 * <roles.h>
 *
 * This file contains the definitions of roles management functions.
 *
 * Note, the copyright+license information is at end of file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "roles.h"

//-- FOR read_capabilities_for_role
#define CAP_FILE_BUFFER_SIZE    4096
#define CAP_FILE_DELIMITERS     ": \t\n"
#define TOK_FLOAT               ","
#define TOK_BAR			":"
#define TOK_SPACE		" "
#define MAX_LEN			512
//-- END for read_capabilities_for_role

/******************************************************************************
 *                      PRIVATE FUNCTIONS DECLARATION                         *
 ******************************************************************************/

static int findSubstr(const char *inpText, const char *pattern);

/* Read the role's capabilities */
static char* read_capabilities_for_role(const char *user, const char *role, 
                                        const char *command);
                                        
/******************************************************************************
 *                      PUBLIC FUNCTIONS DEFINITION                           *
 ******************************************************************************/

/* 
Initialize a user_role_capabilities_t for a given role role, and
for the given user and the groups.
Every entry in the struct is a copy in memory.
The structure must be deallocated with free_urc() afterwards.
Return 0 on success, -1 on failure.
*/
int init_urc(const char *role, const char *user, int nb_groups,
             char **groups, user_role_capabilities_t **urc){
    return init_urc_command(role, NULL, user, nb_groups, groups, urc);
}

/* 
Initialize a user_role_capabilities_t for a given role role,
for a specific command command, and
for the given user and the groups.
Every entry in the struct is a copy in memory.
The structure must be deallocated with free_urc() afterwards.
Return 0 on success, -1 on failure.
*/
int init_urc_command(const char *role, const char *command, const char *user,
                    int nb_groups, char **groups,
                    user_role_capabilities_t **urc){
    int string_len;
    if((*urc = malloc(sizeof(user_role_capabilities_t))) == NULL){
        goto free_on_error;
    }
    //Copy non pointer values and init others
    (*urc)->nb_groups = nb_groups;
    (*urc)->caps.nb_caps = 0;
    (*urc)->caps.capabilities = NULL;
    (*urc)->role = NULL;
    (*urc)->command = NULL;
    (*urc)->user = NULL;
    (*urc)->groups = NULL;

    //Create copy of pointer values
    if(role != NULL){
        string_len = strlen(role) + 1;
        if(((*urc)->role = malloc(string_len * sizeof(char))) == NULL)
            goto free_on_error;
        strncpy((*urc)->role, role, string_len);
    }
    if(command != NULL){
        string_len = strlen(command) + 1;
        if(((*urc)->command = malloc(string_len * sizeof(char))) == NULL)
            goto free_on_error;
        strncpy((*urc)->command, command, string_len);
    }
    if(user != NULL){
        string_len = strlen(user) + 1;
        if(((*urc)->user = malloc(string_len * sizeof(char))) == NULL)
            goto free_on_error;
        strncpy((*urc)->user, user, string_len);
    }
    if(nb_groups > 0){
        char **ptrGpDst, **ptrGpSrc;
        if(((*urc)->groups = calloc(nb_groups, sizeof(char*))) == NULL)
            goto free_on_error;
        for (ptrGpDst = (*urc)->groups, ptrGpSrc = groups; 
                ptrGpDst < (*urc)->groups + nb_groups; ptrGpDst++, ptrGpSrc++){
            int gp_len = strlen(*ptrGpSrc) + 1;
            if((*ptrGpDst = malloc(gp_len * sizeof(char))) == NULL)
                goto free_on_error;
            strncpy(*ptrGpDst, *ptrGpSrc, gp_len);
        }
    }
    return 0;
    
  free_on_error:
    if(*urc != NULL)
        free_urc(*urc);
    return -1;
}

/* 
Deallocate a user_role_capabilities_t
Always return 0.
*/
int free_urc(user_role_capabilities_t *urc){
    if(urc == NULL){
        return 0;
    }
    if(urc->role != NULL)
        free(urc->role);
    if(urc->command != NULL)
        free(urc->command);
    if(urc->user != NULL)
        free(urc->user);
    if(urc->nb_groups > 0 && urc->groups != NULL){
        int i;
        for(i = 0; i < urc->nb_groups; i++){
            if(urc->groups[i] != NULL)
                free(urc->groups[i]);
        }
        free(urc->groups);
    }
    if(urc->caps.nb_caps > 0){
        free(urc->caps.capabilities);
    }
    free(urc);
    return 0;
}

/*
Remove command from urc
This function will be discarded in the new version of config file (xml)
Always return 0;
*/
int remove_urc_command(user_role_capabilities_t *urc){
    if(urc == NULL || urc->command == NULL)
        return 0;
    free(urc->command);
    urc->command = NULL;
    return 0;
}

/* 
Given a initialized user_role_capabilities_t, read the 
capabilityRoles configuration file and fill the structure with the 
matching capabilities.
Return 0 on success, -1 on failure (among them, no matching
capabilities have been found in the configuration file).
*/
int get_capabilities(user_role_capabilities_t *urc){
    char *conf_icaps; //The comma separated list of textual capabilities
    char *cur_caps; 
    int i;
    char *str1;
    char *token;

    //Read the caps from the configfile as a string of comma separated caps
    if(!(conf_icaps = read_capabilities_for_role(urc->user, urc->role,
                                                urc->command))){
        return -1;
    }

    //count the number of capabilities
    urc->caps.nb_caps = 1;
    cur_caps = conf_icaps;
    while((cur_caps = memchr(cur_caps, TOK_FLOAT[0],
                            strlen(cur_caps))) != NULL){
        urc->caps.nb_caps++;
        cur_caps++;
    }

    //Allocate the array of capabilities
    if(!(urc->caps.capabilities = 
            malloc(sizeof(cap_value_t) * urc->caps.nb_caps))){
        perror("Memory allocation error.");
        return -1;
    }

	/* Extract caps in string format from conf_icaps to cap_value_t format */
	for (i = 0,str1 = conf_icaps; ;i++,str1 = NULL) {
		token = strtok(str1, TOK_FLOAT);
		if (token == NULL)
			break;
		cap_from_name(token, urc->caps.capabilities + i);
	}
	return 0;
}

/* 
Printout on stdout a user_role_capabilities_t
*/
void print_urc(const user_role_capabilities_t *urc){
    int i;
    char *cap_name;

    if(urc == NULL){
        printf("URC NULL\n");
        return;
    }
    printf("--- BEGIN URC ---\n");
    printf("Role: ");
    if(urc->role == NULL){
        printf("[None]\n");
    }else{
        printf("%s\n", urc->role);
    }

    printf("User: ");
    if(urc->user == NULL){
        printf("[None]\n");
    }else{
        printf("%s\n", urc->user);
    }

    printf("Groups: ");
    if(urc->nb_groups == 0){
        printf("[None]\n");
    }else{
        for (i=0; i < urc->nb_groups; i++){
            printf("%s ", urc->groups[i]);
        }
        printf("\n");
    }

    printf("Command: ");
    if(urc->command == NULL){
        printf("[None]\n");
    }else{
        printf("%s\n", urc->command);
    }

    printf("Capabilities: ");
    if(urc->caps.nb_caps == 0){
        printf("[None]\n");
    }else{
        for (i=0; i < urc->caps.nb_caps; i++){
            cap_name = cap_to_name(urc->caps.capabilities[i]);
            if(cap_name == NULL){
                printf("Cannot have cap name for %d\n", urc->caps.capabilities[i]);
            }else{
                printf("%d: %s\n", urc->caps.capabilities[i], cap_name);
            }
            cap_free(cap_name);
        }
        printf("\n");
    }
    printf("--- END URC ---\n");
}

/******************************************************************************
 *                      PRIVATE FUNCTIONS DEFINITION                          *
 ******************************************************************************/

static int findSubstr(const char *inpText, const char *pattern) {
    int inplen = strlen(inpText);
    while (inpText != NULL) {

        const char *remTxt = inpText;
        const char *remPat = pattern;

        if (strlen(remTxt) < strlen(remPat)) {
            /* printf ("length issue remTxt %s \nremPath %s \n", remTxt, remPat); */
            return -1;
        }

        while (*remTxt++ == *remPat++) {
            //printf("remTxt %s \nremPath %s \n", remTxt, remPat);
            if (*remPat == '\0') {
                //printf ("match found \n");
                return inplen - strlen(inpText+1);
            }
            if (remTxt == NULL) {
                return -1;
            }
        }
        remPat = pattern;

        inpText++;
    }
}

/* Read the role's capabilities */
static char* read_capabilities_for_role(const char *user, const char *role, 
                                        const char *command){
    char *cap_string = NULL;
    char buffer[CAP_FILE_BUFFER_SIZE], *line;
    FILE *cap_file;

    cap_file = fopen(USER_CAP_FILE_ROLE, "r");
    if (cap_file == NULL) {
        perror("failed to open capability file");
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
            //perror("empty line");
            continue;
        }
        if (*cap_role == '#') {
            //perror("comment line");
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



                    int pos1 = findSubstr(lineTer, "[");

                    if(command && pos1 > -1) {


                        char *echo1 = "echo \"";
                        char *awk1 = "\" | awk -F\'[][]\' \'{print $2}\'";
                        char *commandsafter = malloc(strlen(echo1) + strlen(lineTer) + strlen(awk1) + 1);
                        strcpy(commandsafter,echo1);
                        strcat(commandsafter,lineTer);
                        strcat(commandsafter,awk1);
                        FILE *fId1 = popen(commandsafter, "r");

                        char *commandscutted = malloc(sizeof(fId1));
                        fgets(commandscutted,MAX_LEN,fId1);


                        free(commandsafter);
                        pclose(fId1);





                        int pos = findSubstr(commandscutted, command);
                        if (pos <= -1) {
                            printf("You don't have the right to run this command\n");
                            return NULL;
                        }


                        char *echo = "echo \"";
                        char *awk = "\" | awk -F\'[][]\' \'{print $1}\'";
                        char *usernameafter = malloc(strlen(echo) + strlen(lineTer) + strlen(awk) + 1);
                        strcpy(usernameafter,echo);
                        strcat(usernameafter,lineTer);
                        strcat(usernameafter,awk);
                        FILE *fId = popen(usernameafter, "r");
                        char *usernamecutted = malloc(sizeof(fId));
                        fgets(usernamecutted,MAX_LEN,fId);

                        free(usernameafter);
                        pclose(fId);

                        strcpy(lineTer,usernamecutted);

                        size_t len = strlen(lineTer);

                        lineTer[len-1]='\0';
                    }

                    if (!strcmp(lineTer,user)) {
                        found_one = 1;
                        break;
                    }



                    while ((lineTer = strtok_r(NULL,TOK_FLOAT,&saveptr2)) != NULL) {
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

                    char *gr = "groups ";
                    char *grU = malloc(strlen(gr) + strlen(user) + 1);
                    strcpy(grU,gr);
                    strcat(grU,user);
                    FILE *fGroup = popen(grU,"r");
                    free(grU);
                    //FILE *fGroup = popen("groups $USER","r");
                    char *listString = malloc(sizeof(fGroup));
                    fgets(listString,MAX_LEN,fGroup);
                    pclose(fGroup);

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

                    free(listString);
                    grU = malloc(strlen(gr) + strlen(user) + 1);
                    strcpy(grU,gr);
                    strcat(grU,user);
                    fGroup = popen(grU,"r");
                    free(grU);
                    //fGroup = popen("groups $USER","r");
                    listString = malloc(sizeof(fGroup));
                    fgets(listString,MAX_LEN,fGroup);
                    pclose(fGroup);
                    while ((lineTer = strtok_r(NULL, TOK_FLOAT, &saveptr2)) != NULL) {
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
                }

                cpt++;
                //fprintf(stderr, "user is not [%s] - skipping\n", line);
            }
        }

        cap_role = NULL;
        line = NULL;


        if (found_one) {
            //fprintf(stderr, "user is allowed to use this role\n");
            break;
        }
        if (none) {
            //fprintf(stderr, "user is not allowed to use this role and group are none\n");
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

/* 
 * 
 * Copyright Guillaume Daumas <guillaume.daumas@univ-tlse3.fr>, 2018
 * Copyright Ahmad Samer Wazan <ahmad-samer.wazan@irit.fr>, 2018
 * Copyright Rémi Venant <remi.venant@irit.fr>, 2018
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
