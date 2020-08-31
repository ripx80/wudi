VERSION = 0.1
NAME = wudi

BINDIR = /usr/local/sbin
MANDIR = /usr/share/man/man8
LOG = /var/log/wudi.log

CXX = gcc
CXXFLAGS= -pthread

all: $(NAME)

$(NAME): src/$(NAME).o
	$(CXX) $(CXXFLAGS) src/$(NAME).o -o $(NAME)

$(NAME).o: src/$(NAME).c
	 $(CXX) -c src/$(NAME).c 

install: $(NAME)
	cp $(NAME) $(BINDIR)
	cp man/$(NAME).8.bz2 $(MANDIR)
	
uninstall:
	rm $(BINDIR)/$(NAME)
	rm $(MANDIR)/$(NAME).8.bz2
	if test -e $(LOG); \
	then rm $(LOG); \
	fi

clean:
	rm $(NAME)
	rm src/$(NAME).o

help:
	#use: make [install|uninstall|clean|help]
