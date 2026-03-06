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

/*

The manager loads the file, and reads a fixed number of characters N
every worker gets N characters to check

*/

typedef enum WORD_FLAGS {
    UNDEFINED = 0,
    NOT_WORD = 1,
    
    // these two flags are mutually exclusive
    // one and only one must be set in any word
    START_CAP = 1 << 1,
    START_LOWER = 1 << 2,

    // both are used to check for acronyms
    DOUBLE_CAP = 1 << 3,
    END_CAP = 1 << 4,
} WORD_FLAGS;

typedef struct Word {
    int flags;
    int periodcount; // how many periods, for detecting acronyms
} Word;

/*

What do we do if a worker starts or ends partway into a word?
We don't want to accidentally count extra words

*/

/*

Each worker returns:
* word count
* title case word count
* start word (Word struct)
* end word (Word struct)

start word and end word exist so we can check for words split
on section boundaries, so we don't overcount words

if a worker starts or ends in a word, they don't add it to their word count
the manager must decide if it's a whole word or a split word

start and end words are stored in a queue or queues
we record start and end words for sections sequentially

once we have the end word, E, for section n, and the start word (S)
for section n+1, we analyze them

"having" them means neither is left undefined (at 0)

if both are not words, we don't worry about it, and add nothing to any word count
if both are words, it's one word, so we add one to word count
    we then see if it's a valid title-case word:
    E must start cap
    E and S must not have double capitals
    E period count + S period count < 2
    if E ends cap, S must not start cap

if only one of them is a word, we ignore the other, add one to word count
    check the single word to see if it's a valid title-case word:
    it must start cap
    it must not have double capitols
    period count must be < 2

*/