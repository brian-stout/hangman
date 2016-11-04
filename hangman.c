#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>


int main(void)
{
	const char *name = "HOME";
	char *envptr;

	envptr = getenv(name);

	char home_directory[16];

	strncpy(home_directory, envptr, strlen(envptr));
	strncat(home_directory, "/.hangman", sizeof(home_directory));

	FILE *dictionary = fopen(home_directory, "r");
	if(!dictionary){
		perror("Could not open .words file");
		return EX_NOINPUT;
	}

	size_t line_count = 0;
	int ch;

	while ((ch = fgetc(dictionary)) != EOF){
		if(ch == '\n'){
			++line_count;
		}
	}

	fclose(dictionary);
	dictionary = fopen(home_directory, "r");

	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	printf("%zu\n", line_count);

	while ((read = getline(&line, &len, dictionary)) != -1){
		printf("%s", line);
	}
	
	//getline() automatically mallocs the *line for variable char length 
	free(line);
	fclose(dictionary);
}
