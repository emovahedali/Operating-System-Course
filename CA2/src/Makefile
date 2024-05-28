CC := g++
OBJDIR := objects

CXXFLAGS := -std=c++11

all: main.out linear.out voter.out

main.out: $(OBJDIR)/main.o $(OBJDIR)/logger.o 
	$(CC) $(OBJDIR)/main.o $(OBJDIR)/logger.o -o main.out

linear.out: $(OBJDIR)/linear.o $(OBJDIR)/logger.o $(OBJDIR)/csvhandler.o $(OBJDIR)/data.o
	$(CC) $(OBJDIR)/linear.o $(OBJDIR)/logger.o $(OBJDIR)/csvhandler.o $(OBJDIR)/data.o -o linear.out

voter.out: $(OBJDIR)/voter.o $(OBJDIR)/logger.o  $(OBJDIR)/csvhandler.o
	$(CC) $(OBJDIR)/voter.o $(OBJDIR)/logger.o $(OBJDIR)/csvhandler.o -o voter.out

$(OBJDIR)/main.o: main.cpp logger.cpp logger.hpp color.hpp consts.hpp
	$(CC) -c main.cpp -o $(OBJDIR)/main.o

$(OBJDIR)/linear.o: linear.cpp logger.cpp csvhandler.cpp data.cpp logger.hpp color.hpp consts.hpp csvhandler.hpp data.hpp
	$(CC) -c linear.cpp -o $(OBJDIR)/linear.o

$(OBJDIR)/voter.o: voter.cpp logger.cpp csvhandler.cpp logger.hpp color.hpp consts.hpp csvhandler.hpp 
	$(CC) -c voter.cpp -o $(OBJDIR)/voter.o

$(OBJDIR)/csvhandler.o: csvhandler.cpp logger.cpp csvhandler.hpp logger.hpp
	$(CC) -c csvhandler.cpp -o $(OBJDIR)/csvhandler.o

$(OBJDIR)/logger.o: logger.cpp logger.hpp color.hpp 
	$(CC) -c logger.cpp -o $(OBJDIR)/logger.o

$(OBJDIR)/data.o: data.cpp csvhandler.cpp data.hpp csvhandler.hpp
	$(CC) -c data.cpp -o $(OBJDIR)/data.o

.PHONY: clean

clean:
	rm -r $(OBJDIR)/*.o
	rm *.out
