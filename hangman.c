#define _GNU_SOURCE
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>
#include <stdbool.h>

struct savestate{

	//Variables for the save state
	int wins;
	int losses;
	int miss_amount;
	int winning_streak;
	int losing_streak;
};

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
void result_printer(char *, int, size_t);

void get_letter(char *);

void read_savefile(FILE *, struct savestate *);

void write_savefile(FILE *, struct savestate);

void wipe_string(char *, size_t);

int main(void)
{
	const char *name = "HOME";
	char *envptr;

	envptr = getenv(name);

	char words_directory[32];
	char hangman_directory[32];

	//Copies home directory path to two  buff arrays to avoid writing over env variables
	strncpy(hangman_directory, envptr, sizeof(hangman_directory));
	strncpy(words_directory, envptr, sizeof(words_directory));

	//Cats the correct file names to the end of the directory
	strncat(hangman_directory, "/.hangman", sizeof(hangman_directory));
	strncat(words_directory, "/.words", sizeof(words_directory));

	while(true){
		//Reads the .hangman save file if it exists, if it doesn't initializes it
		FILE *save_file = fopen(hangman_directory, "r");
		if(!save_file){
			save_file = fopen(hangman_directory, "w");
			if(!save_file){
				perror("Can not create .hangman!");
				return EX_CANTCREAT;
			}
			fprintf(save_file, "0\n0\n0\n0\n");
	
			fclose(save_file);
	
			save_file = fopen(hangman_directory, "r");
			if(!save_file){
				perror("Can not open the .hangman file!");
				return EX_NOINPUT;
			}
		}
		struct savestate savestate;
		read_savefile(save_file, &savestate);

		//Opens up the dictionary in the directory, errors out if it's not there
		FILE *dictionary = fopen(words_directory, "r");
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
		dictionary = fopen(words_directory, "r");
	
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
		char word[32] = "\0"; //Initialized default value because valgrind errors
		wipe_string(word, strlen(word));
		//Reads through each line of a file.
		//getline() automatically malocs the *line for a variable char length
		//If the line matches rand_line_number than the loop is broke so we can have our word
		//TODO: Can probably be put in a function
	
		while ((fgets(word, sizeof(word), dictionary)) != EOF){
			++line_count;
			if(line_count == rand_line_number){
				//Removing the newline from the word
				word[strlen(word) - 1] = '\0';
				break;
			}
		}
		size_t len = strlen(word);
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

		//Using a buffer to avoid modifying word directly
		char temp_word[64] = "\0"; //initialized default values because valgrind
		int guess_count = 0;

		while(true){

			//Breaks out of loop if player wins
			if(current_mask == win_mask){
				printf("You win!\n");
				++savestate.wins;
				break;
			}
			//Breaks out of a loop if player makes 6 bad guesses, aka a loss
			if(guess_count == 6){
				printf("You lose!\n");
				++savestate.losses;
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

			result_printer(temp_word, current_mask, len);
			printf("%s\n", temp_word);

			wipe_string(temp_word, len);



		}

		//Wipes out buffers and masks to make rerunning the program more reliable
		result_mask = 0;
		win_mask = 0;
		current_mask = 0;

		fclose(dictionary);
		fclose(save_file);


		save_file = fopen(hangman_directory, "w");
			if(!save_file){
				perror("Can not open .hangman to write to!");
				return EX_NOINPUT;
			}

		write_savefile(save_file, savestate);
		fclose(save_file);

		//TODO: add char to a list of guessed characters
		//TODO: chastize user if he guesses a character already guessed

	}
}


int character_matcher(char string[], char chr)
{
	unsigned int mask = 0;
	char alt_chr = '\0';
	if(isupper(chr)){
		alt_chr = tolower(chr);
	}
	else if (islower(chr)){
		alt_chr = toupper(chr);
	}
	for(size_t i = 0; i < strlen(string); ++i){
		if(string[i] == chr || string[i] == alt_chr || isalpha(string[i]) == 0){
			mask |= 1;
		} 
			mask <<= 1;
	}
	mask >>= 1;
	return mask;
}

void result_printer(char *string, int bitmask, size_t len)
{
	//Starting at end to match the bitmask
	for(int i = len - 1; i >= 0; --i){
		if((bitmask & 1) != 1){
			string[i] = '_';
		}
		bitmask >>= 1;
	}
}

void get_letter(char *chr)
{
	fgets(chr, sizeof(chr), stdin);
	chr[1] = '\0';
}

void read_savefile(FILE *savefile, struct savestate *savestate)
{
		char savebuf[16];
		fgets(savebuf, sizeof(savebuf), savefile);
		savestate->wins = strtol(savebuf, NULL, 10);

		fgets(savebuf, sizeof(savebuf), savefile);
		savestate->losses = strtol(savebuf, NULL, 10);

		fgets(savebuf, sizeof(savebuf), savefile);
		savestate->winning_streak = strtol(savebuf, NULL, 10);

		fgets(savebuf, sizeof(savebuf), savefile);
		savestate->losing_streak = strtol(savebuf, NULL, 10);

}

void write_savefile(FILE *savefile, struct savestate savestate)
{
	fprintf(savefile, "%d\n", savestate.wins);
	fprintf(savefile, "%d\n", savestate.losses);
	fprintf(savefile, "%d\n", savestate.winning_streak);
	fprintf(savefile, "%d\n", savestate.losing_streak);

}

void wipe_string(char *string, size_t length){
	for(size_t i = 0; i < length; ++i){
		string[i] = '\0';
	}
}

