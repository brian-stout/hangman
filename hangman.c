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

/** print_stats function is given the number of guesses and prints out
*		the correct ascii symbols as a response
*/
void print_hangedman(int);


/**	menu_switch handles the logic before and after a game starts allowing
*		the player to reset stats, show stats, and quit the game
*/
int menu_switch(struct savestate *);


int main(void)
{
	//Gets a path to the users home directory
	const char *name = "HOME";
	char *envptr;
	envptr = getenv(name);

	char words_directory[32];
	char hangman_directory[32];

	//Copies home directory path to two  buff arrays to avoid writing
	//over env variables
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

		//Initialized a struct which will keep track of stats
		//Will be passed to a function later to save the state
		struct savestate savestate;

		//Initializes savestate struct
		read_savefile(save_file, &savestate);

		//We're finished with the save_file for now
		fclose(save_file);

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
		while ((fgets(word, sizeof(word), dictionary)) != NULL){
			++line_count;
			if(line_count == rand_line_number){
				//Removing the newline from the word
				word[strlen(word) - 1] = '\0';
				break;
			}
		}

		//Done with the dictionary
		fclose(dictionary);

		while(true){
			int menu_flag = menu_switch(&savestate);
			if(menu_flag == 1){
				break;
			}
			else if(menu_flag == -1){
				return 0;
			}
		}

		//Calculate strlen here because it's used a million times in this function
		size_t word_len = strlen(word);

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
		int miss_count = 0;
		char letter_guess;

		//Using a buffer to avoid modifying word directly
		char temp_word[64] = "\0"; //initialized default values because valgrind

		while(true){

			//Breaks out of loop if player wins
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
				break;
			}
			//Breaks out of a loop if player makes 6 bad guesses, aka a loss
			if(miss_count == 6){
				printf("You lose!  The word was %s\n", word);
				//Saving the results in the savestate struct
				++savestate.losses;
				//Logic determining streaks
				++savestate.losing_streak;
				if(savestate.losing_streak > 1){
					printf("You are on a %d game losing streak!\n"
							" ", savestate.losing_streak);
				}
				//Sets winning streak to 0 on loss
				if(savestate.winning_streak > 0){
					savestate.winning_streak = 0;
				}
				break;
			}

			print_stats(savestate);			

			//Getting the charactere here
			printf("Guess a letter: ");
			letter_guess = get_letter();
			printf("\n");  //New line for formatting

			//Figures out how many characters are in the word 
			//and returns the resulting mask
			result_mask = character_matcher(word, letter_guess, word_len);
	
			//Checks for a bad guess
			if(result_mask == miss_mask){
				printf("Bad guess!\n");
				++miss_count;
				++savestate.misses;
			}

			print_hangedman(miss_count);

			//Or's the current mask so program can keep track of player progress
			current_mask |= result_mask;

			//Copying the word into a temp_word because result_printer
			//modifies the array that's passed to it
			strncpy(temp_word, word, word_len);

			//Prints out the string the user will see and use to play
			result_printer(temp_word, current_mask, word_len);
			printf("%d: %s\n", miss_count, temp_word);

			//Wipe temp_word for program stability
			wipe_string(temp_word, word_len);
		}

		//Wipes out buffers and masks to make rerunning the program more stable
		win_mask = 0;
		current_mask = 0;
		word_len = 0;

		//Open the file back up so we can save the results of the round
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
	//initializes alt_char to avoid conditional with unitialized value
	char alt_chr = '\0';
	//Logic to make program case insensitive
	if(isupper(chr)){
		alt_chr = tolower(chr);
	}
	else if (islower(chr)){
		alt_chr = toupper(chr);
	}
	for(size_t i = 0; i < word_len; ++i){
		//If char matches or is punctuation set bit to 1
		if(string[i] == chr || string[i] == alt_chr || isalpha(string[i]) == 0){
			mask |= 1;
		} 
			mask <<= 1;
	}
	//Handles the null byte
	mask >>= 1;
	return mask;
}


void result_printer(char *string, int bitmask, size_t word_len)
{
	//Starting at end to match the bitmask
	for(int i = word_len - 1; i >= 0; --i){
		if((bitmask & 1) != 1){
			//If bit set to 0 then it'll display a _
			string[i] = '_';
		}
		//Shift right to go back through the bits
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

	//Entire thing will look like this
	//Game 8.   1 win/7 losses. Average score: 0

	printf("Game %d.   ", total_games);
	//Handles grammar for wins
	if(savestate.wins > 1){
		printf("%d wins/", savestate.wins);
	}
	else{
		printf("%d win/", savestate.wins);
	}
	//Handles grammar for losses
	if(savestate.losses == 1){
		printf("%d loss. ", savestate.losses);
	}
	else{
		printf("%d losses. ", savestate.losses);
	}
	
	if(total_games == 0){
		printf("Welcome to hangman!\n");
	}
	//Handles a floating point exception
	else if(savestate.misses > 0){
		printf("Average score: %d\n", (total_games/savestate.misses));	
	}
	else{
		printf("Average score: %d.0\n", total_games);
	}	
}


void print_hangedman(int miss_count)
{
	if(miss_count >= 1){
		printf("  O\n");
	}
	if(miss_count == 2 ){
		printf("  |\n"); 
	}
	else if(miss_count == 3)
	{
		printf(" /|\n");
	}
	else if(miss_count > 3){
		printf(" /|\\ \n");
	}
	if(miss_count == 5){
		printf(" /\n"); 
	}
	else if(miss_count == 6){
		printf(" / \\\n");
	}
}


int menu_switch(struct savestate *savestate)
{
	char num_switch = '1';

	printf("\nWhat would you like to do?\n"
			"1.  Play a game  2.  Show stats  "
			"\n3.  Reset Stats  4.  Quit\n");

	printf("Option: ");
	num_switch = get_letter();
	printf("\n");
	
	//Switches cause they're for cool kids
	switch(num_switch) {
		case '1' :
			return 1;
			break;
		case '2' :
			print_stats(*savestate);
			return 0;
			break;
		case '3' :
			savestate->wins = 0;
			savestate->losses = 0;
			savestate->winning_streak = 0;
			savestate->losing_streak = 0;
			savestate->misses = 0;
			printf("Resetting your stats!\n");
			break;
		case '4' :
			return -1;
			break;
		//Because it's reflex
		case 'q' :
			return -1;
			break;
		//Default for bad input
		default :
			printf("Please enter a valid option!\n");
			break;
	}
	return 0;
}
