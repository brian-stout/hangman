TARGET = hangman
$(TARGET):

.PHONY: clean 

clean:
	rm $(TARGET)
    
CPPFLAGS+=-Wall -Wextra -Wpedantic -Wwrite-strings -Wstack-usage=1024 -Wfloat-equal -Waggregate-return -Winline

CFLAGS+=-std=c11 

