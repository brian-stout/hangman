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
}
