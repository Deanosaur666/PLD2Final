/*

Determine the frequencies of title-cased words
(proper nouns and sentence starters) in a large
English text file, such as a book. Exclude acronyms
from consideration. Use a manager-worker architecture.

*/

/*

BOOKS:
* Dracula.md
* Frankenstein.md
* Hound.md

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

If a word does not start with an NOT_IN_WORDuppercase,
we won't check if it's an acronym.
We don't care. We only count title cases and total words.

*/

#define ALPHANUMERIC(c) ((c >= 'a' && c <= 'z') \
                        || (c >= 'A' && c <= 'Z') \
                        || (c >= '0' && c <= '9'))

// could this character be the start of a word?
#define START_WORD(c) (ALPHANUMERIC(c))
// could this character be part of a word?
#define IN_WORD(c) (ALPHANUMERIC(c) || c == '\'' || c == '.')

#define UPPERCASE(c) (c >= 'A' && c <= 'Z')

/*

The manager loads the file, and reads a fixed number of characters N
every worker gets N characters to check

*/

#define BUFF_MAX 1024

void readSection(char * text, int * wordCount, int * titleCount, int * acronymCount) {

    bool inWord = false;
    bool startCap = false;
    int periodCount;

    bool acronym = false;

    int i = 0;
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

int main (int argc, char *argv[]) {

    char buffer[BUFF_MAX];
    int wordCount = 0;
    int titleCount = 0;
    int acronymCount = 0;

    // argument should be the file
    FILE *file = fopen("Frankenstein.md", "r");

    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Reading lines until the end of the file
    int read = fread(buffer, sizeof(char), BUFF_MAX - 1, file);
    while (read != 0) {
        // null terminate it
        buffer[read] = '\0';
        // shorten the buffer if we end in the middle of a word
        while(IN_WORD(buffer[read - 1])) {
            buffer[read - 1] = '\0';
            fseek(file, -1, SEEK_CUR);
            read --;

            if(read <= 0)
                return 0;
        }
        readSection(buffer, &wordCount, &titleCount, &acronymCount);

        printf("%s", buffer);
        printf("|%d|", BUFF_MAX - read);
        read = fread(buffer, sizeof(char), BUFF_MAX, file);
    }

    fclose(file);

    printf("\nWords: %d\nTitles: %d\nAcronyms: %d\n", wordCount, titleCount, acronymCount);

    return 0;
}