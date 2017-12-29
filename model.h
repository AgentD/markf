#ifndef MODEL_H
#define MODEL_H

#include <stddef.h>

typedef struct touple_t touple_t;
typedef struct word_t word_t;


enum {
	TYPE_WORD = 0x01,		/* A regular word */
	TYPE_COMMA = 0x02,		/* ',' */
	TYPE_COLON = 0x03,		/* ':' */
	TYPE_SEMICOL = 0x04,		/* ';' */
	TYPE_DOT = 0x05,		/* '.' */
	TYPE_QUOTE = 0x06,		/* '"' */
	TYPE_SQUOTE = 0x07,		/* '\'' */
	TYPE_PAR_OPEN = 0x08,		/* '(' */
	TYPE_PAR_CLOSE = 0x09,		/* ')' */
	TYPE_BRA_OPEN = 0x0A,		/* '[' */
	TYPE_BRA_CLOSE = 0x0B,		/* ']' */
	TYPE_BRACE_OPEN = 0x0C,		/* '{' */
	TYPE_BRACE_CLOSE = 0x0D,	/* '}' */
	TYPE_BANG = 0x0E,		/* '!' */
	TYPE_QUESTION = 0x0F,		/* '?' */
	TYPE_MINUS = 0x10,		/* '-' */

	TYPE_START = 0x11,		/* start of file */
	TYPE_EOF = 0x12			/* end of file */
};

struct word_t {
	int type;
	unsigned char *word;	/* if type == TYPE_WORD, a string */

	unsigned int id;

	touple_t *followup;	/* list of words that followed this one */

	word_t *next;		/* linked list */
};


struct touple_t {
	word_t *word;		/* the follow up word */
	size_t count;		/* how often that was the follow up word */

	touple_t *next;		/* lined list */
};

#endif /* MODEL_H */

