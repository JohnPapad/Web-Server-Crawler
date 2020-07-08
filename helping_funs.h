#ifndef HELPING_FUNS_H_INCLUDED
#define HELPING_FUNS_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <fcntl.h>
#include <limits>
#include <sys/wait.h>
#include <poll.h>

using namespace std;



char * read_cmd(int fd, bool &err);
char * get_parameter(char ** argv, int argc, const char * par_flag);
bool is_unsigned_int_number(const char * str);
bool is_IP(const char *str);
void sep_term_line();
int dir_exists(char * in_dir);
FILE * open_file(char * fn);
char ** strip_string_into_lines(char * str, unsigned int & n_lines);
char ** strip_line_into_words(char * cmd, unsigned int & cmd_nwords);
void delete_str_array(char ** str_array, unsigned int array_size);
char * parse_file(FILE * fp);
char * get_timestamp();
char * get_http_timestamp();
char * ms_to_time(int duration);
unsigned int get_number_of_digits(int num);
bool send_message(int fd, const char * response);
char * get_stats_str(bool server, unsigned int pages, unsigned int bytes, char * elapsed_time_str);
void bind_on_port(int sock, int port);
//--------------------------------------------------------------------
char * make_full_file_path(char * path, char * text_file_name);
char ** parse_text_file(FILE * fp, unsigned int & n_lines, unsigned int & n_chars);
char ** parse_text_file(FILE * fp, unsigned int & n_lines, unsigned int & n_chars);
int scan_dir_for_files(char * in_dir);

int send_msg(const char * msg, unsigned int fd);
char * read_msg(unsigned int fd);
int send_int_number(unsigned int fd, int number);
int read_int_number(unsigned int fd);



#endif // HELPING_FUNS_H_INCLUDED
