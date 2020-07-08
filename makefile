CC=g++
CXXFLAGS= -Wall -g -std=c++11

.PHONY: all myhttpd mycrawler

all: myhttpd mycrawler

#-----------------------------------------------------------------------------------
myhttpd: server_main.o myhttpd.o helping_funs.o
	$(CC) $(CXXFLAGS) server_main.o myhttpd.o helping_funs.o -o myhttpd -lpthread
	
server_main.o: server_main.cpp helping_funs.h myhttpd.h
	$(CC) $(CXXFLAGS) -c server_main.cpp
	
myhttpd.o: myhttpd.cpp myhttpd.h
	$(CC) $(CXXFLAGS) -c myhttpd.cpp
	
helping_funs.o: helping_funs.cpp helping_funs.h
	$(CC) $(CXXFLAGS) -c helping_funs.cpp
	
#-----------------------------------------------------------------------------------

mycrawler: crawler_main.o mycrawler.o helping_funs.o start_search.o jobexec.o worker.o search_answer.o dir.o text_file.o trie.o posting_list.o
	$(CC) $(CXXFLAGS) crawler_main.o mycrawler.o helping_funs.o start_search.o jobexec.o worker.o search_answer.o dir.o text_file.o trie.o posting_list.o -o mycrawler -lpthread
	
crawler_main.o: crawler_main.cpp helping_funs.h mycrawler.h
	$(CC) $(CXXFLAGS) -c crawler_main.cpp
	
start_search.o: start_search.cpp
	$(CC) $(CXXFLAGS) -c start_search.cpp
		
jobexec.o: jobexec.cpp jobexec.h helping_funs.h
	$(CC) $(CXXFLAGS) -c jobexec.cpp
		
worker.o: worker.cpp worker.h helping_funs.h
	$(CC) $(CXXFLAGS) -c worker.cpp
	
search_answer.o: search_answer.cpp search_answer.h helping_funs.h
	$(CC) $(CXXFLAGS) -c search_answer.cpp
	
dir.o: dir.cpp dir.h helping_funs.h
	$(CC) $(CXXFLAGS) -c dir.cpp
	
text_file.o: text_file.cpp text_file.h helping_funs.h
	$(CC) $(CXXFLAGS) -c text_file.cpp
	
trie.o: trie.cpp trie.h helping_funs.h
	$(CC) $(CXXFLAGS) -c trie.cpp
	
posting_list.o: posting_list.cpp posting_list.h dir.h helping_funs.h
	$(CC) $(CXXFLAGS) -c posting_list.cpp
	
.PHONY: cleanall clean1 clean2 cleanfifos

cleanall: clean1 clean2

clean1:
	rm -f myhttpd server_main.o myhttpd.o helping_funs.o
	
clean2:
	rm -f mycrawler crawler_main.o mycrawler.o helping_funs.o start_search.o jobexec.o worker.o search_answer.o dir.o text_file.o trie.o posting_list.o *.fifo
	
cleanfifos:
	rm -f *.fifo
    
	
