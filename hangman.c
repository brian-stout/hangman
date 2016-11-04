#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>


int main(void)
{
	const char *name = "HOME";
	char *envptr;

	envptr = getenv(name);

	char home_directory[16];

	//Copies whats in envptr to home_directory buffer so we don't write over env variables
	strncpy(home_directory, envptr, strlen(envptr));

	//Makes full directory path to .hangman file
	strncat(home_directory, "/.hangman", sizeof(home_directory));

	FILE *dictionary = fopen(home_directory, "r");
	if(!dictionary){
		perror("Could not open .words file");
		return EX_NOINPUT;
	}

	int line_count = 0;
	int ch;

	//Gets the line count to be used to pick a random line later
	//TODO: Could be implemented into a function
	while ((ch = fgetc(dictionary)) != EOF){
		if(ch == '\n'){
			++line_count;
		}
	}

	//Closes and reopens the file so it can be read again
	fclose(dictionary);
	dictionary = fopen(home_directory, "r");

	//Sets the seed for the random number generator
	srand(time(NULL));
	int rand_line_number;

	//Determines which line will be pulled out from the file.
	//The +1 includes the final number
	//TODO: Can be put in function.. maybe unnesscary?
	rand_line_number = rand() % line_count + 1;

	//Sets line_count to zero again so it can be reused
	line_count = 0;

	//Prepping getline function
	char *word = NULL;
	size_t len = 0;

	//Reads through each line of a file.
	//getline() automatically malocs the *line for a variable char length
	//If the line matches rand_line_number than the loop is broke so we can have our word
	//TODO: Can probably be put in a function
	while ((getline(&word, &len, dictionary)) != -1){
		++line_count;
		if(line_count == rand_line_number){
			//Removing the newline from the word
			word[strlen(word) - 1] = '\0';
			break;
		}
	}
	printf("DEBUG: %s\n", word);
	
	//making sure to free line because it was malloc'd
	free(word);
	fclose(dictionary);
}
