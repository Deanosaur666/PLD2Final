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

*/

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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <mpi.h>

#define START_CAP 1
#define START_LOWER 0
#define NOT_WORD -1

typedef struct Word {
    int startType; // did it start with a capital letter?
    // 0 = no, 1 = yes, -1 = not a word
    int twocaps; // two caps in a row, for detecting acronyms
    // boolish type, 0 = no, 1 = yes
    int periodcount; // how many periods, for detecting acronyms
} Word;

/*

What do we do if a worker starts or ends partway into a word?
We don't want to accidentally count extra words
All books start with a hash symbol (#), so we always start
in a "not in word" state

*/

#define NOT_TITLE 0
#define MAYBE_TITLE 1 // we don't know if it's an acronym yet