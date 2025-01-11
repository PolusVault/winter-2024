CC = clang
CFLAGS = -std=c17 -Wall -Wpedantic -Wstrict-aliasing -I.

# store *.o in objs dir, probably better to put everything in /build
OBJDIR = objs
# https://www.gnu.org/software/make/manual/html_node/Text-Functions.html
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(wildcard *.c))
BIN = app

.PHONY: dirs all clean

all: dirs $(BIN)

dirs:
	mkdir -p objs

$(BIN): $(OBJS)
	$(CC) -o ./$@ $^

$(OBJDIR)/%.o : %.c
	$(CC) $(CFLAGS) -MMD -MF $(OBJDIR)/$*.d -o $@ -c $<

-include $(OBJDIR)/*.d

clean:
	rm -rf $(BIN) $(OBJS)






