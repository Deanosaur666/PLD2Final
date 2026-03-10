/*

Determine the frequencies of title-cased words
(proper nouns and sentence starters) in a large
English text file, such as a book. Exclude acronyms
from consideration. Use a manager-worker architecture.

*/

/*

BOOKS:
* Hound.md          (59501 words)
* Frankenstein.md   (75208 words)
* Dracula.md        (161865 words)

These books were obtained from Project Gutenberg,
downloaded as html files, and converted to markdown files
using the python program html-to-markdown

I converted words like naïveté to naivete,
to make the reading process simpler

To be more precise, I converted UTF-8 to ASCII

I used this command to convert the files:
iconv -f UTF-8 -t ASCII//TRANSLIT -o out.md in.md

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <mpi.h>

/*

WORDS:
A word starts when we hit any alphanumeric character (a-z, A-Z, 0-9)
The numeric is important because 7th is a word, for example.
A word ends when it hits any other type of character,
except an apostrophe (') or a period (.)
This is important because of contractions like doesn't or won't
and because of acronyms like C.C.H.

We exclude acronyms like M.R.C.S., but include words like "I" or "A"
we also include words like "Dr." that end in a period

How to determine if something is an acronym
A word is an acronym if it contains more than one period (C.C.H.)
A word is an acronym if it contains two capitals in a row (CIA)

A word like McDonald is not an acronym.
It contains two capitols, but they are seperated by a lowercase.

*/

#define ALPHANUMERIC(c) ((c >= 'a' && c <= 'z') \
                        || (c >= 'A' && c <= 'Z') \
                        || (c >= '0' && c <= '9'))

// could this character be the start of a word?
#define START_WORD(c) (ALPHANUMERIC(c))
// could this character be part of a word?
#define IN_WORD(c) (ALPHANUMERIC(c) || c == '\'' || c == '.')
// is this character uppercase?
#define UPPERCASE(c) (c >= 'A' && c <= 'Z')

/*

The manager loads the file, and reads a fixed number of characters N.
Every worker gets N characters to check.

*/

#define BUFF_MAX 1024

// manager function, used to get text from a file, into the buffer
// this buffer will then be sent from the manager to a worker
int readSection(char * buffer, FILE * file) {
    // read from file
    int read = fread(buffer, sizeof(char), BUFF_MAX - 1, file);
    if(read == 0)
        return 0;
    // null terminate it
    buffer[read] = '\0';
    // shorten the buffer if we end in the middle of a word
    while(IN_WORD(buffer[read - 1])) {
        buffer[read - 1] = '\0';
        fseek(file, -1, SEEK_CUR);
        read --;

        if(read <= 0) {
            printf("Error reading file (buffer too small).\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            fclose(file);
            exit(1);
            return -1;
        }
    }

    return read;
}

// worker function
// this is the meat of the program, where we actually count words
void countSection(char * text, int * wordCount, int * titleCount, int * acronymCount) {

    // are we currently in a word?
    bool inWord = false;
    // did this word start with a capital?
    bool startCap = false;
    // how many periods in this word (for acronym detection)?
    int periodCount;
    // is this word an acronym?
    bool acronym = false;

    // iteration
    int i = 0;
    // which letter of this word are we on?
    int letter = 0;

    while(i < BUFF_MAX) {
        char c = text[i];
        // first letter of word
        if(!inWord && START_WORD(c)) {
            inWord = true;
            letter = 0;
            periodCount = 0;
            acronym = false;
            if(UPPERCASE(c))
                startCap = true;
        }
        // continue the word
        else if(inWord && IN_WORD(c)) {
            letter ++;
            if(c == '.') {
                if(periodCount >= 2)
                    acronym = true;
                else
                    periodCount ++;
            }
            // if the second letter is uppercase
            else if(UPPERCASE(c) && letter == 1)
                acronym = true;
        }
        // end of word
        else if(inWord) {
            inWord = false;
            if(acronym)
                (*acronymCount) ++;
            else {
                (*wordCount) ++;
                if(startCap)
                    (*titleCount) ++;
            }
            startCap = false;
        }

        // break at null
        if(c == '\0')
            break;
        i ++;
    }
}

#define TAG_TEXT 0
#define TAG_END 1

int main (int argc, char *argv[]) {
    int p; // number of processors
    int id; // process ID number
    double  elapsed_time;   /* Parallel execution time */
    
    FILE * file = NULL; // file to read
    char buffer[BUFF_MAX]; // worker's buffer
    int readChars = -1; // number of characters read from file
    
    // counting is what we do best
    int wordCount = 0;
    int titleCount = 0;
    int acronymCount = 0;

    int global_wordCount = 0;
    int global_titleCount = 0;
    int global_acronymCount = 0;

    // stack for unsent buffers, so manager can work ahead
    int stackMax = 0; // maximum size of stack, will be set to p or something
    int stackSize = 0;
    char ** textStack = NULL;

    char ** sentText = NULL;

    // requests and stuff
    MPI_Request * send_requests = NULL;

    // for text and end
    MPI_Request text_recv_request;
    MPI_Request end_recv_request;


    MPI_Init (&argc, &argv);

    MPI_Comm_rank (MPI_COMM_WORLD, &id);
    MPI_Comm_size (MPI_COMM_WORLD, &p);
    MPI_Barrier(MPI_COMM_WORLD);

    if(argc < 2) {
        if(!id)
            printf ("Command line: %s <text file>\n", argv[0]);
        MPI_Finalize();
        exit(1);
    }

    /* Start the timer */
    elapsed_time = -MPI_Wtime();

    // argument should be the file
    // only p0 reads the file
    if(!id) {
        file = fopen(argv[1], "r");

        if (file == NULL) {
            printf("Error opening file.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }
    }
    // sequential algorithm with one process
    if(!id && p == 1) {
        // Reading lines until the end of the file
        readChars = readSection(buffer, file);
        while (readChars != 0) {
            countSection(buffer, &wordCount, &titleCount, &acronymCount);
            readChars = readSection(buffer, file);
        }

        // my counts are the only counts
        global_wordCount = wordCount;
        global_titleCount = titleCount;
        global_acronymCount = acronymCount;
    }
    // manager
    else if(!id) {
        // set up stack
        stackMax = p;
        textStack = malloc(stackMax * sizeof(char *));
        for(int i = 0; i < stackMax; i ++) {
            textStack[i] = malloc(BUFF_MAX * sizeof(char));
        }

        // sent text
        sentText = malloc(p * sizeof(char *));
        for(int i = 0; i < p; i ++) {
            sentText[i] = NULL; // nothing sent yet
        }

        // requests
        send_requests = malloc(p * sizeof(MPI_Request));
        
        // manager main loop
        while(true) {
            int working = 0;

            // add to stack
            if(readChars != 0 && stackSize < stackMax) {
                readChars = readSection(textStack[stackSize], file);
                if(readChars != 0)
                    stackSize ++;
            }
            // send to workers who are free
            if(stackSize > 0) {
                for(int i = 1; i < p; i ++) {
                    if(sentText[i] == NULL && stackSize > 0) {
                        // pop from stack
                        stackSize --;
                        sentText[i] = textStack[stackSize];
                        MPI_Isend(sentText[i], BUFF_MAX, MPI_CHAR, i, TAG_TEXT, MPI_COMM_WORLD, &send_requests[i]);
                        // pave over old stack top buffer so we can reuse it
                        textStack[stackSize] = malloc(BUFF_MAX * sizeof(char));
                    }
                }
            }
            // test for workers ready to recieve
            for(int i = 1; i < p; i ++) {
                if(sentText[i] != NULL) {
                    int done = 0;
                    MPI_Test(&send_requests[i], &done, MPI_STATUS_IGNORE);
                    if(done) {
                        free(sentText[i]);
                        sentText[i] = NULL;
                    }
                    else {
                        working ++; // a worker is working
                    }
                }
            }

            // nobody is working, nothing in the stack to send, and nothing left to read
            // let's finish stuff up
            if(working == 0 && stackSize == 0 && readChars == 0) {
                for(int i = 1; i < p; i ++) {
                    // send "END" message, to let workers know we're closing up shop
                    MPI_Send(NULL, 0, MPI_INT, i, TAG_END, MPI_COMM_WORLD);
                }

                break;
            }
        }
    }
    // worker
    else {
        // prepare to recieve "END" message
        MPI_Irecv(NULL, 0, MPI_INT, 0, TAG_END, MPI_COMM_WORLD, &end_recv_request);
        // worker main loop
        while(true) {
            // prepare to recieve next block of text
            MPI_Irecv(buffer, BUFF_MAX, MPI_CHAR, 0, TAG_TEXT, MPI_COMM_WORLD, &text_recv_request);

            int done = 0;
            int type = -1;
            // test for either more text or END message
            while(true) {
                MPI_Test(&text_recv_request, &done, MPI_STATUS_IGNORE);
                if(done) {
                    type = TAG_TEXT;
                    break;
                }
                MPI_Test(&end_recv_request, &done, MPI_STATUS_IGNORE);
                if(done) {
                    type = TAG_END;
                    break;
                }
            }
            // do work, keep counts
            if(type == TAG_TEXT) {
                countSection(buffer, &wordCount, &titleCount, &acronymCount);
            }
            // break when program is finished
            else if(type == TAG_END) {
                break;
            }
        }
    }
    
    // done managing, done working
    // get global counts
    MPI_Reduce (&wordCount, &global_wordCount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce (&titleCount, &global_titleCount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce (&acronymCount, &global_acronymCount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if(!id)
        fclose(file);
    
    float freq = (float) global_titleCount / global_wordCount;

    /* Stop the timer */
    elapsed_time += MPI_Wtime();

    /* Print the results */
    if (!id) {
        printf("wc:\t\t%8d words\ntc:\t\t%8d title-case words\nac:\t\t%8d acronyms\n", global_wordCount, global_titleCount, global_acronymCount);
        printf("tc freq:\t%8.2f %% of words are title-case\n", freq * 100);
        printf ("T:\t\t%8.6f s\n", elapsed_time);
    }

    // cleanup
    MPI_Finalize();

    if(!id && p > 1) {
        free(send_requests);
        free(sentText);
        for(int i = 0; i < stackMax; i ++) {
            free(textStack[i]);
        }
        free(textStack);
    }

    return 0;
}