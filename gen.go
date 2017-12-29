package main

import (
	"github.com/thoj/go-ircevent"
	"math/rand"
	"strings"
	"unicode"
	"bytes"
	"sort"
	"time"
	"fmt"
	"os"
)

const (
	TYPE_WORD = 0x01		/* A regular word */
	TYPE_COMMA = 0x02		/* ',' */
	TYPE_COLON = 0x03		/* ':' */
	TYPE_SEMICOL = 0x04		/* ';' */
	TYPE_DOT = 0x05			/* '.' */
	TYPE_QUOTE = 0x06		/* '"' */
	TYPE_SQUOTE = 0x07		/* '\'' */
	TYPE_PAR_OPEN = 0x08		/* '(' */
	TYPE_PAR_CLOSE = 0x09		/* ')' */
	TYPE_BRA_OPEN = 0x0A		/* '[' */
	TYPE_BRA_CLOSE = 0x0B		/* ']' */
	TYPE_BRACE_OPEN = 0x0C		/* '{' */
	TYPE_BRACE_CLOSE = 0x0D		/* '}' */
	TYPE_BANG = 0x0E		/* '!' */
	TYPE_QUESTION = 0x0F		/* '?' */
	TYPE_MINUS = 0x10		/* '-' */

	TYPE_START = 0x11		/* start of file */
	TYPE_EOF = 0x12			/* end of file */
)

type Touple struct {
	Word int		/* the follow up word */
	Count int		/* how often that was the follow up word */
}

type Word struct {
	Type int
	Word string

	FollowupTotal int

	Followup []Touple
}


var word_count_min int
var word_count_max int
var words []Word


func readU8(f *os.File) int {
	b := make([]byte, 1)

	if _, err := f.Read(b); err != nil {
		panic(err)
	}

	return int(b[0])
}

func readU16(f *os.File) int {
	b := make([]byte, 2)

	if _, err := f.Read(b); err != nil {
		panic(err)
	}

	return int(b[0]) | (int(b[1]) << 8)
}

func readU32(f *os.File) int {
	b := make([]byte, 4)

	if _, err := f.Read(b); err != nil {
		panic(err)
	}

	return int(b[0]) | (int(b[1]) << 8) |
		(int(b[2]) << 16) | (int(b[3]) << 24)
}

func readModel(filename string) {
	f, err := os.Open(filename)
	if err != nil {
		panic(err)
	}

	defer f.Close()

	word_count_min = readU32(f)
	word_count_max = readU32(f)
	num_words := readU32(f)

	words = make([]Word, num_words)

	for i := 0; i < num_words; i++ {
		words[i].Type = int(readU8(f))
		words[i].FollowupTotal = 0

		if slen := readU8(f); slen != 0 {
			raw := make([]byte, slen)

			if _, err := f.Read(raw); err != nil {
				panic(err)
			}

			words[i].Word = string(raw)
		}

		num_tupples := readU16(f)
		if num_tupples == 0 {
			words[i].Followup = nil
			continue
		}

		words[i].Followup = make([]Touple, num_tupples)

		for j := 0; j < num_tupples; j++ {
			words[i].Followup[j].Word = readU32(f);
			words[i].Followup[j].Count = readU32(f);

			words[i].FollowupTotal += words[i].Followup[j].Count
		}

		sort.Slice(words[i].Followup,
				func(a, b int) bool {
					return words[i].Followup[a].Count >
						words[i].Followup[b].Count
				})
	}
}

func pickWord(current, prefereEof int) int {
	if words[current].FollowupTotal > 0 {
		r := rand.Intn(words[current].FollowupTotal)
		cumulative := 0

		for _, t := range words[current].Followup {
			cumulative += t.Count

			if words[t.Word].Type == TYPE_EOF {
				if prefereEof < 0 {
					continue
				}
				if prefereEof > 0 {
					return t.Word
				}
			}

			if r < cumulative {
				return t.Word
			}
		}
	}

	return -1
}

const (
	FLAG_QUOTE = 0x01
	FLAG_PAREN = 0x02
)

func generate() string {
	var buffer bytes.Buffer

	count := 0
	flags := 0
	current := 0
	last := 0

	for idx, wrd := range words {
		if wrd.Type == TYPE_START {
			current = idx
			break
		}
	}

	if words[current].Type != TYPE_START {
		fmt.Errorf("cannot find start word!\n")
		os.Exit(1)
	}

	rand.Seed(time.Now().UnixNano())

	for {
		prefereEof := 0

		if count < word_count_min {
			prefereEof = -1
		} else if count > word_count_max {
			prefereEof = 1
		}

		last = current
		current = pickWord(last, prefereEof)
		if current < 0 || words[current].Type == TYPE_EOF {
			break
		}

		/* print out */
		switch words[current].Type {
		case TYPE_WORD:
			if words[last].Type == TYPE_WORD {
				buffer.WriteString(" ")
			}
			buffer.WriteString(words[current].Word)
		case TYPE_COMMA:
			buffer.WriteString(", ")
		case TYPE_COLON:
			buffer.WriteString(": ")
		case TYPE_SEMICOL:
			buffer.WriteString("; ")
		case TYPE_DOT:
			buffer.WriteString(". ")
		case TYPE_QUOTE:
			if flags & FLAG_QUOTE != 0 {
				buffer.WriteString("\" ")
			} else {
				buffer.WriteString(" \"")
			}
			flags ^= FLAG_QUOTE
		case TYPE_SQUOTE:
			buffer.WriteString("'")
		case TYPE_PAR_OPEN:
			flags |= FLAG_PAREN
			buffer.WriteString(" (");
		case TYPE_PAR_CLOSE:
			flags &= ^FLAG_PAREN
			buffer.WriteString(") ");
		case TYPE_BRA_OPEN:
			buffer.WriteString(" [")
		case TYPE_BRA_CLOSE:
			buffer.WriteString("] ")
		case TYPE_BRACE_OPEN:
			buffer.WriteString(" {")
		case TYPE_BRACE_CLOSE:
			buffer.WriteString("} ")
		case TYPE_BANG:
			buffer.WriteString("! ")
		case TYPE_QUESTION:
			buffer.WriteString("? ")
		case TYPE_MINUS:
			buffer.WriteString("-")
		}

		count++
	}

	if flags & FLAG_QUOTE != 0 {
		buffer.WriteString("\"")
	}
	if flags & FLAG_PAREN != 0 {
		buffer.WriteString(")")
	}

	return buffer.String()
}


const irc_nick = ""
const irc_name = ""
const irc_server = ""
const irc_channel = ""

const nickserv_pw = ""



var next_sentence = ""

func precache() {
	next_sentence = generate()
	fmt.Println(next_sentence)
	fmt.Println("------------------")
}


func main() {
	if len(os.Args) != 2 {
		fmt.Errorf("usage: gen <model file>\n")
		os.Exit(1)
	}

	readModel(os.Args[1])

	irccon := irc.IRC(irc_nick, irc_name)
	irccon.UseTLS = true

	irccon.AddCallback("001",
			func(e *irc.Event) {
				irccon.Privmsgf("NickServ", "identify %s",
						nickserv_pw)
			})

	irccon.AddCallback("NOTICE",
			func(e *irc.Event) {
				if e.Nick != "NickServ" {
					return 
				}

				if !strings.HasPrefix(e.Message(), "You are now identified") {
					return
				}

				precache()
				irccon.Join(irc_channel)
			})

	irccon.AddCallback("PRIVMSG",
		func(e *irc.Event) {
			if e.Arguments[0] == irc_channel {
				msg := e.Message()

				fields := strings.FieldsFunc(msg,
					func(c rune) bool {
						if c == ',' || c == ':' ||
						   c == '.' || c == '?' ||
						   c == '!' {
							return true
						}
						return unicode.IsSpace(c)
					})

				respond := false

				for _, f := range fields {
					if f == irc_nick {
						respond = true
						break
					}
				}

				if respond {
					time.Sleep(1000 * time.Millisecond)
					irccon.Privmsg(irc_channel,
							next_sentence)
					precache()
				}
			} else {
				time.Sleep(1000 * time.Millisecond)
				irccon.Privmsg(e.Nick, next_sentence)
				precache()
			}
		})

	if err := irccon.Connect(irc_server); err != nil {
		panic(err)
	}

	irccon.Loop()
}
