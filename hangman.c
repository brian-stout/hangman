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
	int misses;
};


/**	A function that passes a string and a character
*		The function iterates through each character of the string, and if
*		the characters match up, it sets the flag to 1 and returns a bit mask
*		for all the places it was correct.
*
*		If function return a zero, the player had a miss
*		If function returns a full bit mask, the player wins.
*/
int character_matcher(char *, char, size_t);


/**	A function that passes a string and a bit mask
*		The function iterates through each character of a string and
*		if the bit in the mask is set to 1, will print out that character,
*		else it'll print out an underscore unless it's a nonalphabet character
*
*		TODO: Write logic to test for unicode and handle it properly
*/
void result_printer(char *, int, size_t);


/**	get_letter function is responsible for receiving user input
*		as well as error handling user input
*/
char get_letter();

/** read_savefile function reads the savefile ".hangman" that the
*		program generates, parses that information and passes it to
*		the savestate struct which keeps track of all the user stat
*		while user runs the program
*/
void read_savefile(FILE *, struct savestate *);

/** write_savefile function takes the savestate struct before the program
*		terminates and writes it to .hangman in the 
*
*
*/
void write_savefile(FILE *, struct savestate);

/** simple function that takes in a pointer to a string and fills it with
*		null bytes.  The purpose is to wipe buffers at the end of a playthrough
*		to improve program stability when running multiple times.
*
*
*/
void wipe_string(char *, size_t);


/**	print_stats function prints the first line before the guess.
*		It's entire purpose is to condense a bunch of logic to
*		make the main program less noisy.
*
*		Handles grammar as well.
*/
void print_stats(struct savestate);

int win_check(int, int, struct savestate *);


int main(void)
{
	//Gets a path to the users home directory
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
			fprintf(save_file, "0\n0\n0\n0\n0\n");
	
			fclose(save_file);
	
			save_file = fopen(hangman_directory, "r");
			if(!save_file){
				perror("Can not open the .hangman file!");
				return EX_NOINPUT;
			}
		}

		//Initialized a struct that will keep track of stats
		//Will be passed to a function later to save the states on round completion
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
		//TODO: Can probably be put in a function
		while ((fgets(word, sizeof(word), dictionary)) != EOF){
			++line_count;
			if(line_count == rand_line_number){
				//Removing the newline from the word
				word[strlen(word) - 1] = '\0';
				break;
			}
		}

		//Calculate strlen here because it's used a million times in this function
		size_t word_len = strlen(word);
		printf("DEBUG: %s\n", word);

		char letter_guess;

		//Generates a number that is used to compare to a full mask aka a win
		unsigned int win_mask = 1;
		win_mask <<= word_len;
		win_mask -= 1;

		//Defines what a miss is.
		//An empty character_matcher result plus punctuation
		unsigned int miss_mask = character_matcher(word, '\0', word_len);
	
		//Intializes values to be used in the loop later
		unsigned int current_mask = 0;
		unsigned int result_mask;

		//Using a buffer to avoid modifying word directly
		char temp_word[64] = "\0"; //initialized default values because valgrind
		int guess_count = 0;

		while(true){

			//Breaks out of loop if player wins
			if(win_check(current_mask, win_mask, &savestate)){
				break;
			}

			//Breaks out of a loop if player makes 6 bad guesses, aka a loss
			if(guess_count == 6){
				printf("You lose!\n");
				++savestate.losses;
				++savestate.losing_streak;
				if(savestate.losing_streak > 1){
					printf("You are on a %d game losing streak!\n"
							" ", savestate.losing_streak);
				}
				if(savestate.winning_streak > 0){
					savestate.winning_streak = 0;
				}
				break;
			}

			print_stats(savestate);			

			//Gets character
			printf("Guess a letter: ");
			letter_guess = get_letter();

			//Captures a result mask and ors it with the current mask

			//Figures out how many characters are in the word and returns the resulting mask
			result_mask = character_matcher(word, letter_guess, word_len);
	
			//Checks for a bad guess
			if(result_mask == miss_mask){
				printf("Bad guess!\n");
				++guess_count;
				++savestate.misses;
			}

			//Or's the current mask so program can keep track of player progress
			current_mask |= result_mask;

			//Creating a temporary array for word so function doesn't modify the original word
			strncpy(temp_word, word, word_len);

			result_printer(temp_word, current_mask, word_len);
			printf("%s\n", temp_word);

			wipe_string(temp_word, word_len);
		}

		//Wipes out buffers and masks to make rerunning the program more stable
		result_mask = 0;
		win_mask = 0;
		current_mask = 0;
		word_len = 0;

		fclose(dictionary);
		fclose(save_file);

		save_file = fopen(hangman_directory, "w");
			if(!save_file){
				perror("Can not open .hangman to write to!");
				return EX_NOINPUT;
			}

		write_savefile(save_file, savestate);
		fclose(save_file);
	}
}


int character_matcher(char string[], char chr, size_t word_len)
{
	unsigned int mask = 0;
	char alt_chr = '\0';
	if(isupper(chr)){
		alt_chr = tolower(chr);
	}
	else if (islower(chr)){
		alt_chr = toupper(chr);
	}
	for(size_t i = 0; i < word_len; ++i){
		if(string[i] == chr || string[i] == alt_chr || isalpha(string[i]) == 0){
			mask |= 1;
		} 
			mask <<= 1;
	}
	mask >>= 1;
	return mask;
}


void result_printer(char *string, int bitmask, size_t word_len)
{
	//Starting at end to match the bitmask
	for(int i = word_len - 1; i >= 0; --i){
		if((bitmask & 1) != 1){
			string[i] = '_';
		}
		bitmask >>= 1;
	}
}


char get_letter()
{
	char buf[32];
	fgets(buf, sizeof(buf), stdin);
	return(buf[0]);
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

		fgets(savebuf, sizeof(savebuf), savefile);
		savestate->misses = strtol(savebuf, NULL, 10);

}


void write_savefile(FILE *savefile, struct savestate savestate)
{
	fprintf(savefile, "%d\n", savestate.wins);
	fprintf(savefile, "%d\n", savestate.losses);
	fprintf(savefile, "%d\n", savestate.winning_streak);
	fprintf(savefile, "%d\n", savestate.losing_streak);
	fprintf(savefile, "%d\n", savestate.misses);
}


void wipe_string(char *string, size_t word_len){
	for(size_t i = 0; i < word_len; ++i){
		string[i] = '\0';
	}
}

void print_stats(struct savestate savestate)
{
	//Variable calculated to increase readability
	int total_games = savestate.losses + savestate.wins;
	printf("Game %d.   ", total_games);
	if(savestate.wins > 2){
		printf("%d wins//", savestate.wins);
	}
	else{
		printf("%d win//", savestate.wins);
	}
	if(savestate.losses > 2){
		printf("%d losses. ", savestate.losses);
	}
	else{
		printf("%d loss. ", savestate.losses);
	}
	printf("Average score: %d\n", (total_games/savestate.misses));		
}

int win_check(int current_mask, int win_mask, struct savestate *savestate)
{
	if(current_mask == win_mask){
		printf("You win!\n");
		++savestate.wins;
		++savestate.winning_streak;
		if(savestate.winning_streak > 1){
			printf("You are on a %d game winning streak!\n"
					" ", savestate.winning_streak);
		}
		if(savestate.losing_streak > 0){
			savestate.losing_streak = 0;
		}
		return true;
	}
}
