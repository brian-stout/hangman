#define _GNU_SOURCE
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>

/* A function that passes a string and a character
	The function iterates through each character of the string, and if
	the characters match up, it sets the flag to 1 and returns a bit mask
	for all the places it was correct.

	If function return a zero, the player had a miss
	If function returns a full bit mask, the player wins.
*/
int character_matcher(char *, char);


/*  A function that passes a string and a bit mask
	The function iterates through each character of a string and
	if the bit in the mask is set to 1, will print out that character,
	else it'll print out an underscore unless it's a nonalphabet character

	TODO: Write logic to test for unicode and handle it properly

*/
void result_printer(char *, int);

void get_letter(char *);

int main(void)
{
	const char *name = "HOME";
	char *envptr;

	envptr = getenv(name);

	char home_directory[16];

	//Copies whats in envptr to home_directory buffer so we don't write over env variables
	strncpy(home_directory, envptr, strlen(envptr));

	//Makes full directory path to .hangman file
	strncat(home_directory, "/.words", sizeof(home_directory));

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
	printf("DEBUG: There are %d lines in this file\n", line_count);
	printf("DEBUG: The random line is %d\n", rand_line_number);

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


	char letter_guess;

	//Generates a number that is used to compare to a full mask aka a win
	unsigned int win_mask = 1;
	win_mask <<= strlen(word);
	win_mask -= 1;

	//Defines what a miss is.  A character_matcher result including punctuation
	unsigned int miss_mask = character_matcher(word, '\0');

	//Intializes values to be used in the loop later
	unsigned int current_mask = 0;
	unsigned int result_mask;

	//Automatically assigns memory for a buffer so word won't be modified by the print function
	char *temp_word = (char *) malloc(strlen(word));
	int guess_count = 0;

	while(1){

		//Breaks out of loop if player wins
		if(current_mask == win_mask){
			printf("You win!\n");
			break;
		}
		//Breaks out of a loop if player makes 6 bad guesses, aka a loss
		if(guess_count == 6){
			printf("You lose!\n");
			break;
		}
		
		//Gets character
		//TODO: do error handling for get_letter()
		printf("Guess a letter: ");
		get_letter(&letter_guess);

		//Captures a result mask and ors it with the current mask

	//Figures out how many characters are in the word and returns the resulting mask
		result_mask = character_matcher(word, letter_guess);

		//Checks for a bad guess
		if(result_mask == miss_mask){
			printf("Bad guess!\n");
			++guess_count;
		}

		//Or's the current mask so program can keep track of player progress
		current_mask |= result_mask;


	//Creating a temporary array for word so function doesn't modify the original word
		strncpy(temp_word, word, strlen(word));
		result_printer(temp_word, current_mask);

		printf("%s\n", temp_word);
	}

	
	
	//making sure to free line because it was malloc'd
	free(word);
	free(temp_word);
	fclose(dictionary);

	//TODO: Get a char from the user and run it through the character_matcher
	//TODO: add char to a list of guessed characters
	//TODO: chastize user if he guesses a character already guessed
}


int character_matcher(char string[], char chr)
{
	unsigned int mask = 0;
	char alt_chr;
	if(isupper(chr)){
		alt_chr = tolower(chr);
	}
	else if (islower(chr)){
		alt_chr = toupper(chr);
	}
	for(size_t i = 0; i < strlen(string); ++i){
		if(string[i] == chr || string[i] == alt_chr || isalpha(string[i]) == 0){
			mask  |= 1;
		} 
			mask <<= 1;
	}
	mask >>= 1;
	return mask;
}

void result_printer(char *string, int bitmask)
{
	for(int i = strlen(string) - 1; i >= 0; --i){
		if((bitmask & 1) != 1){
			string[i] = '_';
		}
		bitmask >>= 1;
	}
}

void get_letter(char *chr)
{
	fgets(chr, sizeof(chr), stdin);
}
