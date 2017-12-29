CFLAGS := -ansi -pedantic -Wall -Wextra

.PHONY: all
all: model gen

.PHONY: clean
clean:
	$(RM) model gen

model: model.c model.h
#gen: gen.c model.h

gen: gen.go
	go get github.com/thoj/go-ircevent
	go build -o $@ $^
