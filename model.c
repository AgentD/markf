#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include <unistd.h>
#include <fcntl.h>

#include "model.h"


static word_t *words = NULL;
static word_t *last_word = NULL;
static size_t word_count = 0;
static size_t word_count_min = 0;
static size_t word_count_max = 0;


static int add_followup(word_t *w)
{
	touple_t *t;

	if (last_word != NULL) {
		for (t = last_word->followup; t != NULL; t = t->next) {
			if (t->word == w) {
				t->count += 1;
				break;
			}
		}

		if (t == NULL) {
			t = malloc(sizeof(*t));
			if (t == NULL) {
				fputs("out of memory\n", stderr);
				return -1;
			}

			t->next = last_word->followup;
			last_word->followup = t;

			t->word = w;
			t->count = 1;
		}
	}

	last_word = w;
	return 0;
}

static int add_word(int type, const unsigned char *word)
{
	size_t len = 0;
	word_t *w;

	if (type == TYPE_WORD)
		len = strlen((const char *)word) + 1;

	for (w = words; w != NULL; w = w->next) {
		if (w->type != type)
			continue;

		if (type == TYPE_WORD && memcmp(w->word, word, len) != 0)
			continue;

		break;
	}

	if (w == NULL) {
		w = malloc(sizeof(*w));
		if (w == NULL)
			goto fail_oom;

		w->type = type;
		w->word = NULL;
		w->followup = NULL;
		w->next = words;
		words = w;

		if (type == TYPE_WORD) {
			w->word = malloc(len);
			if (w->word == NULL)
				goto fail_oom;

			memcpy(w->word, word, len);
		}
	}

	word_count += 1;
	return add_followup(w);
fail_oom:
	fputs("out of memory\n", stderr);
	return -1;
}

static void reset_words(void)
{
	touple_t *t;
	word_t *w;

	while (words != NULL) {
		w = words;
		words = words->next;

		while (w->followup != NULL) {
			t = w->followup;
			w->followup = t->next;

			free(t);
		}

		free(w->word);
		free(w);
	}
}

/****************************************************************************/

static void sanitize_word(unsigned char *word)
{
	while (*word) {
		if (*word < 0x80) {
			if (isupper(*word))
				*word = tolower(*word);
		}

		++word;
	}
}

static int process_input_file(const char *filename, int infile)
{
	unsigned char c, buffer[128];
	int ret, i = 0, last_was_eof = 0;

	last_word = NULL;
	word_count = 0;

	if (add_word(TYPE_START, NULL))
		return -1;

	for (;;) {
		ret = read(infile, &c, 1);

		if (ret < 0) {
			if (errno == EINTR)
				continue;
			perror(filename);
			return -1;
		}

		if (ret == 0)
			break;

		if (isalnum(c) || c >= 0x80 || c == '*' || c == '-' ||
		    c == '_' || c == '|' ||
		    (i == 0 && (c == '!' || c == ':' || c == '/'))) {
			if (i == 0 && c < 0x80 && !isgraph(c))
				continue;

			if ((size_t)i == sizeof(buffer)) {
				buffer[i - 1] = '\0';
				fprintf(stderr, "%s: word '%s...' is "
						"TOO DAMN LONG\n",
						filename, buffer);
				return -1;
			}

			buffer[i++] = c;
		} else {
			if (last_was_eof) {
				last_was_eof = 0;

				if (add_word(TYPE_START, NULL))
					return -1;
			}

			if (i > 0) {
				if (c == ':') {
					i = 0;
					continue;
				}

				buffer[i] = '\0';
				sanitize_word(buffer);
				if (add_word(TYPE_WORD, buffer))
					return -1;
				i = 0;
			}

			switch (c) {
			case ',': ret = add_word(TYPE_COMMA, NULL); break;
			case ':': ret = add_word(TYPE_COLON, NULL); break;
			case ';': ret = add_word(TYPE_SEMICOL, NULL); break;
			case '.': ret = add_word(TYPE_DOT, NULL); break;
			case '"': ret = add_word(TYPE_QUOTE, NULL); break;
			case '\'': ret = add_word(TYPE_SQUOTE, NULL); break;
			case '(': ret = add_word(TYPE_PAR_OPEN, NULL); break;
			case ')': ret = add_word(TYPE_PAR_CLOSE, NULL); break;
			case '[': ret = add_word(TYPE_BRA_OPEN, NULL); break;
			case ']': ret = add_word(TYPE_BRA_CLOSE, NULL); break;
			case '{': ret = add_word(TYPE_BRACE_OPEN, NULL); break;
			case '}': ret = add_word(TYPE_BRACE_CLOSE, NULL); break;
			case '!': ret = add_word(TYPE_BANG, NULL); break;
			case '?': ret = add_word(TYPE_QUESTION, NULL); break;
			case '-': ret = add_word(TYPE_MINUS, NULL); break;
			case '\n':
				ret = add_word(TYPE_EOF, NULL);

				if (!word_count_min && !word_count_max) {
					word_count_min = word_count;
					word_count_max = word_count;
				} else {
					if (word_count < word_count_min)
						word_count_min = word_count;
					if (word_count > word_count_max)
						word_count_max = word_count;
				}

				last_was_eof = 1;
				word_count = 0;
				last_word = NULL;
				break;
			default:
				ret = 0;
				break;
			}

			if (ret)
				return -1;
		}
	}

	return 0;
}

/****************************************************************************/

static ssize_t write_u8(int outfd, unsigned int value)
{
	uint8_t u = value;
	return write(outfd, &u, sizeof(u));
}

static ssize_t write_u16(int outfd, unsigned int value)
{
	uint16_t v = value;
	return write(outfd, &v, sizeof(v));
}

static ssize_t write_u32(int outfd, unsigned int value)
{
	uint32_t v = value;
	return write(outfd, &v, sizeof(v));
}

static void dump_model(int outfd)
{
	touple_t *t;
	word_t *w;
	size_t i;

	for (i = 0, w = words; w != NULL; w = w->next, ++i)
		w->id = i;

	write_u32(outfd, word_count_min);
	write_u32(outfd, word_count_max);
	write_u32(outfd, i);			/* number of unique words */

	for (w = words; w != NULL; w = w->next) {
		write_u8(outfd, w->type);

		if (w->type == TYPE_WORD) {
			i = strlen((const char *)w->word);

			write_u8(outfd, i);
			write(outfd, w->word, i);
		} else {
			write_u8(outfd, 0);
		}

		for (i = 0, t = w->followup; t != NULL; t = t->next, ++i)
			;

		write_u16(outfd, i);

		for (t = w->followup; t != NULL; t = t->next) {
			write_u32(outfd, t->word->id);
			write_u32(outfd, t->count);
		}
	}
}

int main(int argc, char **argv)
{
	int i, infile;

	for (i = 1; i < argc; ++i) {
		infile = open(argv[i], O_RDONLY);

		if (infile < 0) {
			perror(argv[i]);
			goto fail;
		}

		if (process_input_file(argv[i], infile)) {
			close(infile);
			goto fail;
		}

		close(infile);
	}

	dump_model(1);

	reset_words();
	return EXIT_SUCCESS;
fail:
	reset_words();
	return EXIT_FAILURE;
}
