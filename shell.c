#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

/** Simple UNIX Shell
 *  @author Nefari0uss
 *  @license MIT
 *
 *  To quit, enter quit or exit.
 *  `r` will run a history command.
 *  `h` will show the history.
 **/ 

/** Definitions **/
#define MAXLINE 80 /* 80 chars per line, per command, should be enough. */

/** Global Variables **/
int HISTORY_SIZE = 12;
int HISTORY_FILE_SIZE = 0;
int HISTORY_COUNT = 1;
int OUT_HISTORY_CHANGED = 0;
FILE *fp;

/** Struct - Implementing a Linked List **/
typedef struct HistoryNode
{
	int count;
	char buffer[MAXLINE];

	// Pointer to the next item in the list
	struct HistoryNode *next;
} history_node;


/** Function Prototypes **/
void addToHistory(history_node **head, char buffer[]);
void removeDuplicates(history_node **head, history_node **newNode);
void viewHistory(history_node **head);
void setHistorySize(char *args[], history_node **head);
void setup(char iBuffer[], char *args[], int *bgrnd, char buffer[]);
int child(char iBuffer[], char *args[]);
void quit(history_node **head);
void executeCommand(char iBuffer[], char *args[], int bgrnd);
int mostRecentCommand(char buffer[], history_node **head);
void modifiedSetup(char iBuffer[], char *args[], int *bgrnd, char buffer[]);		
int rHistory(char buffer[], char *args[], history_node **head);
void adjustForSize(history_node **head);
void initializeHistoryFile(history_node **head, char buffer[]);
void setOutHistorySize(char *args[]);

/** Main **/
int main(void)
{
	/* Variables */
	char iBuffer[MAXLINE]; /* Buffer to hold the command entered */
	char buffer[MAXLINE]; /* Holds complete input - unmodified. */
	char *args[MAXLINE/2+1];/* Command line (of 80) has max of 40 arguments */
	int bgrnd;             /* Equals 1 if a command is followed by '&', else 0 */

	/* Begin initialization of linked list. */
	history_node *head = NULL;

	initializeHistoryFile(&head, buffer);


	while(1) {            /* Program terminates normally inside setup */

		bgrnd = 0;

		printf("\nSys2ShC: ");  /* Shell prompt */
		fflush(0);

		setup(iBuffer, args, &bgrnd, buffer);       /* Get next command */

		// If input is a r history command. Either rr or r something.
		int historyCommand = 1;
		if (strncmp(iBuffer, "rr", 2) == 0) {
			//printf("Recent history.\n");
			historyCommand = mostRecentCommand(buffer, &head);
			if (historyCommand) {
				modifiedSetup(iBuffer, args, &bgrnd, buffer);
			}
		} else if (strncmp(iBuffer, "r", 1) == 0) {
			historyCommand = rHistory(buffer, args, &head);
			if (historyCommand) {
				modifiedSetup(iBuffer, args, &bgrnd, buffer);
			}
		}

		//printf("Printing buffer: %s \n", iBuffer);

		// If input is something else.
		if (strncmp(iBuffer, "history", 7) == 0 || strncmp(iBuffer, "h", 1) == 0) {
			addToHistory(&head, buffer);
			viewHistory(&head);
		} else if (strncmp(iBuffer, "sethistoryout", 13) == 0) {
			addToHistory(&head, buffer);
			setOutHistorySize(args);
		} else if (strncmp(iBuffer, "sethistory", 10) == 0) {
			addToHistory(&head, buffer);
			setHistorySize(args, &head);
		} else if (strncmp(iBuffer, "quit", 4) == 0 || strncmp(iBuffer, "exit", 4) == 0) {
			quit(&head);
		} else if (strncmp(iBuffer, "", 1) == 0) {
			/** Do nothing. Blank input. **/
		} else /** Create child **/ {
			// Adjustment based on whether history command had an error or is already run.
			if (historyCommand) {
				addToHistory(&head, buffer);
				executeCommand(iBuffer, args, bgrnd);
			}
		} // end if-else
	} // end while
}


/** The setup() routine reads in the next command line string storing it in the input buffer.
  The line is separated into distinct tokens using whitespace as delimiters.  Setup also
  modifies the args parameter so that it holds points to the null-terminated strings which
  are the tokens in the most recent user command line as well as a NULL pointer, indicating the
  end of the argument list, which comes after the string pointers that have been assigned to
  args. ***/

void setup(char iBuffer[], char *args[], int *bgrnd, char buffer[])
{
	int length,  /* #  characters in the command line */
	    i,       /* Index for iBuffer arrray          */
	    start,   /* Beginning of next command parameter           */
	    j;       /* Where to place the next parameter into args[] */

	/* Read what the user enters */

	fgets(iBuffer, MAXLINE, stdin);
	length = strlen(iBuffer);

	strcpy(buffer, iBuffer);


	j = 0;
	start = -1;

	if (length == 0)
		exit(0);            /* Cntrl-d was entered, end of user command stream */

	if (length < 0){
		perror("error reading command");
		exit(-1);           /* Terminate with error code of -1 */
	}

	/* Examine every character in the input buffer */
	for (i = 0; i < length; i++) {

		switch (iBuffer[i]){
			case ' ':
			case '\t' :          /* Argument separators */

				if(start != -1){
					args[j] = &iBuffer[start];    /* Set up pointer */
					j++;
				}

				iBuffer[i] = '\0'; /* Add a null char; make a C string */
				start = -1;
				break;

			case '\n':             /* Final char examined */
				if (start != -1){
					args[j] = &iBuffer[start];
					j++;
				}

				iBuffer[i] = '\0';
				args[j] = NULL; /* No more arguments to this command */
				break;

			case '&':
				*bgrnd = 1;
				iBuffer[i] = '\0';
				break;

			default :             /* Some other character */
				if (start == -1)
					start = i;
		}

	}
	args[j] = NULL; /* Just in case the input line was > 80 */
}

int child(char iBuffer[], char *args[]) {
	int execute = execvp(iBuffer, args);
	if (execute == -1) {
		printf("Error: No such file, directory, or command.\n");
	}
	exit(0);
}

/** Frees memory and quits the program. */
void quit(history_node **head) {
	history_node *ptr = *head;
	history_node *current = *head;

	if (OUT_HISTORY_CHANGED == 1) {
		HISTORY_SIZE = HISTORY_FILE_SIZE;
		//printf("History size is set to history file size: %d\n", HISTORY_SIZE);
	}
	//adjustForSize(&(*head));
	adjustForSize(&ptr);


	if (*head != NULL) {

		while (ptr != NULL) {
			current = ptr;
			ptr = ptr->next;
			fprintf(fp, "%d %s", current->count, current->buffer);
			free(current);
		}
	}
	fclose(fp);
	exit(0);
}

/** Loops through history and prints results to terminal. **/
void viewHistory(history_node **head) {
	history_node *ptr = *head;
	if (*head == NULL || (*head)->count == 0) {
		printf("Nothing in history.\n");
	} else {
		while (ptr != NULL) {
			printf("%d %s", ptr->count, ptr->buffer);
			ptr = ptr->next;
		}
	}
}

/** Adds entry to history then makes modifications based on size and removes duplicates. */
void addToHistory(history_node **head, char buffer[]) {

	/** Create a history node with the entry info. **/
	history_node *newNode = malloc(sizeof(history_node));
	newNode->count = HISTORY_COUNT;
	strcpy(newNode->buffer, buffer);
	newNode->next = NULL;

	/** Update the history count **/
	HISTORY_COUNT++;

	/** If head is NULL, new node is the first value. Otherwise add new node to end of list. **/
	//printf("History_Size is currently: %d \n", HISTORY_SIZE);
	/** History size of 0 means no history is to be stored at all. **/
	if (HISTORY_SIZE == 0) {
		//printf("History size is 0.\n");
		free(*head);
		//printf("Head value: %s \n",(*head)->buffer);
	} else if(*head == NULL){
		*head = newNode;
		//viewHistory(&(*head));
	} else if(strcmp(newNode->buffer, (*head)->buffer) == 0 && ((*head)->next == NULL)) {
		//printf("First node is a duplicate.\n");
		*head = newNode;
		//viewHistory(&(*head));
	} else {

		/** Add newNode to the end of the linked list. **/
		history_node *ptr = *head;
		while (ptr->next != NULL) {
			ptr = ptr->next;
		}
		ptr->next = newNode;

		/* Adjust linked list by removing any duplicates and then making modifications based on history size. */
		removeDuplicates(&(*head), &newNode);
		adjustForSize(&(*head));
		//viewHistory(&(*head));
	}
}

/** Modify list based on history size.**/
void adjustForSize(history_node **head) {
	//printf("Adjusting linked list for size.\n");

	int minNode = HISTORY_COUNT - HISTORY_SIZE;
	/**
	  if (HISTORY_COUNT > HISTORY_SIZE) {
	//minNode -= 1;
	} else {
	//minNode += 1;
	}
	 **/ 
	//printf("history count: %d \t history size: %d \t ", historyCount, HISTORY_SIZE); 
	//printf("Nodes that should be in list are %d. \n", minNode);
	if (minNode > 0) {
		while (*head != NULL && (*head)->count < minNode) {
			//printf("%d %s", (*head)->count, (*head)->buffer);
			*head = (*head)->next;
		}
	}
}

/** Remove any duplicate entries in the list. **/
void removeDuplicates(history_node **head, history_node **newNode) {
	//printf("Removing duplicates...\n");
	history_node *ptr = *head;
	history_node *previous = *head;

	int loop = 1; // Loop index. Used to check which pass the loop is on.
	// Loop until newNode. New node is the last on in the list.
	while (ptr->next != NULL) {
		//printf("Checking for duplicates. Pass %d. ", loop);

		/** Check for duplicate.**/
		if (strcmp(ptr->buffer, (*newNode)->buffer) == 0) {
			/** Check if duplicate is the head or not. **/
			if (strcmp((*head)->buffer, (*newNode)->buffer) == 0) {
				//printf("Head is duplicate.\n");
				*head = (*head)->next;
				ptr = *head;
				previous = *head;
				break;
				// check if ptr->next is now null?
			} else {
				//printf("Found duplicate - is not head.\n");
				previous->next = ptr->next;
				ptr = ptr->next;
				break;
			} // end nested if-else
		} else /** No duplicate found. **/ {
			//printf("No duplicate.\n");
			previous = ptr;
			ptr = ptr->next;
		} // end if-else
		loop++;
	} // end while
}

/** Adjust the history size then adjust the list accordingly. **/
void setHistorySize(char *args[], history_node **head) {
	//printf("%s \n", args[1]);
	HISTORY_SIZE = atoi(args[1]);
	if (HISTORY_SIZE < 0) {
		printf("History cannot be negative. ");
		HISTORY_SIZE = 0;
	}
	printf("History Size is now set to: %d \n", HISTORY_SIZE);
	adjustForSize(&(*head));
}

/** Execute the command entered by the user if possible. **/
void executeCommand(char iBuffer[], char *args[], int bgrnd) {

	pid_t childPid = fork();

	if (childPid == 0) {
		child(iBuffer, args);
	} else if (childPid < 0) {
		printf("Error creating child process.\n");
	}

	/** If background process **/
	if (bgrnd == 0) {
		int returnStatus;
		waitpid(childPid, &returnStatus, 0);
	} // end if
}

/** Get the last command entered by the user and run it again. **/
int mostRecentCommand(char buffer[], history_node **head) {
	//printf("Getting most recent command.\n");

	history_node *ptr = *head;
	if (ptr == NULL) {
		printf("No recent commands.");
		return 0;
	} else {
		while (ptr->next != NULL) {
			//printf("%d %s", ptr->count, ptr->buffer);
			ptr = ptr->next;
		}
		//printf("ptr buffer is %s \n", ptr->buffer);
		strcpy(buffer, ptr->buffer);
		return 1;
	}
}

/** Rerun set up based on some pre-determined input - aka, buffer. **/
void modifiedSetup(char iBuffer[], char *args[], int *bgrnd, char buffer[]) {

	int length,  /* #  characters in the command line */
	    i,       /* Index for iBuffer arrray          */
	    start,   /* Beginning of next command parameter           */
	    j;       /* Where to place the next parameter into args[] */

	/* Read what the user enters */

	strcpy(iBuffer, buffer);
	length = strlen(iBuffer);

	strcpy(buffer, iBuffer);


	j = 0;
	start = -1;

	if (length == 0)
		exit(0);            /* Cntrl-d was entered, end of user command stream */

	if (length < 0){
		perror("error reading command");
		exit(-1);           /* Terminate with error code of -1 */
	}

	/* Examine every character in the input buffer */
	for (i = 0; i < length; i++) {

		switch (iBuffer[i]){
			case ' ':
			case '\t' :          /* Argument separators */

				if(start != -1){
					args[j] = &iBuffer[start];    /* Set up pointer */
					j++;
				}

				iBuffer[i] = '\0'; /* Add a null char; make a C string */
				start = -1;
				break;

			case '\n':             /* Final char examined */
				if (start != -1){
					args[j] = &iBuffer[start];
					j++;
				}

				iBuffer[i] = '\0';
				args[j] = NULL; /* No more arguments to this command */
				break;

			case '&':
				*bgrnd = 1;
				iBuffer[i] = '\0';
				break;

			default :             /* Some other character */
				if (start == -1)
					start = i;
		}

	}
	args[j] = NULL; /* Just in case the input line was > 80 */


}

/** Get a command based on some r entry. If r num, get the r num if possible. If r string, get the latest matching string entry possible. **/
int rHistory(char buffer[], char *args[], history_node **head) {
	//printf("Getting most r history.\n");

	history_node *ptr = *head;
	char *cPtr;
	int number;
	char bufferTemp[MAXLINE];
	strcpy(bufferTemp, buffer);

	/** Check for inapproriate inputs or if there are no commands in the history. **/
	if (args[1] == NULL) {
		printf("Must include a number or string after r.\n");
		return 0;
	} else if (*head == NULL) {
		printf("No commands in history.\n");
		return 0;
	}

	/** Parse input string and ints. **/
	number = (int) strtol(args[1], &cPtr, 10);
	//printf("cPtr is %s \n", cPtr);

	/** If the string is NULL or empty, assumption is that it is r num. **/
	if (cPtr == NULL || *cPtr == '\0') {
		//printf("Looking at int.\n");

		/** Check if num < 0**/
		if (number <= 0) {
			printf("Number after r must be positive.\n");
			return 0;
		}

		/** Check if num = 1 **/
		if ((*head)->next == NULL && (*head)->count == number) {
			strcpy(buffer, ptr->buffer);

			//viewHistory(&(*head));
			return 1;
		}

		/** Check if num > 1**/
		while (ptr->next != NULL) {
			if (ptr->count == number) {
				strcpy(buffer, ptr->buffer);
				//viewHistory(&(*head));
				return 1;
			} // end if
			ptr = ptr->next;
		} // end while
		printf("Could not find command in history. \n");
		return 0;
	} else /* cPtr contains a string */ { /** Do r string. **/
		//printf("Looking at string.\n");			
		int matchFound = 0;
		int strLength = strlen(cPtr); 

		/** Loop through linked list. **/
		while (ptr->next != NULL) {
			/** Check if match. If so, store it and keep checking. Latest entry will be kept. **/
			if (strncmp(ptr->buffer, cPtr, strLength) == 0) {
				strcpy(bufferTemp, ptr->buffer);
				matchFound = 1;
			} // end if
			ptr = ptr->next;
		} // end while

		if (matchFound == 1) {
			strcpy(buffer, bufferTemp);
			return 1;
		}

		printf("Could not find command in history. \n");
		return 0;
	} // end outer if-else
}

/** Initialize history file if it doesn't exist otherwise read from it. **/
void initializeHistoryFile(history_node **head, char buffer[]) {
	fp = fopen("history.shell", "r");

	if (fp == NULL) {
		printf("Creating history file.\n");
		fp = fopen("history.shell", "a+");
	} else {
		printf("Reading from history file.\n");
		//fp = fopen("history.shell", "a+");

		while(fgets(buffer, MAXLINE, fp)) {
			HISTORY_FILE_SIZE++;
			if (HISTORY_FILE_SIZE > HISTORY_SIZE) {
				HISTORY_SIZE++;
			}


			/** Parse input string and ints. **/
			char *cPtr;
			int number = (int) strtol(buffer, &cPtr, 10);
			HISTORY_COUNT = number;

			/** Remove starting space. **/
			if (cPtr[0] == ' ') {
				cPtr = cPtr + 1;
			}

			addToHistory(&(*head), cPtr);
		}
		fclose(fp);
		fopen("history.shell", "w+");
	}
}

/** Set history file size. **/
void setOutHistorySize(char *args[]) {
	//printf("%s \n", args[1]);
	HISTORY_FILE_SIZE = atoi(args[1]);
	if (HISTORY_FILE_SIZE < 0) {
		printf("History cannot be negative. ");
		HISTORY_FILE_SIZE = 0;
	}
	printf("History Out Size is now set to: %d \n", HISTORY_FILE_SIZE);
	OUT_HISTORY_CHANGED = 1;
}
