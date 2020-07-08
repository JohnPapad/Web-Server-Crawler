#ifndef MYCRAWLER_H_INCLUDED
#define MYCRAWLER_H_INCLUDED

#include "helping_funs.h"

#define POOL_SIZE 150

typedef struct
{
    char ** used_urls;   // store all urls that have been requested from server
    char ** curr_urls;   // store urls to be requested from server
    int used_urls_size;
    int curr_urls_size;
} pool_t;

typedef struct
{
    struct sockaddr * serverptr;
    int server_size;
    char * host_str;
    char * save_dir;
} client_arg;

extern pthread_mutex_t mtx;
extern pthread_cond_t cond_nonempty;
extern pool_t pool;
extern client_arg cl_arg;

extern pthread_mutex_t mtx2;

extern unsigned int pages_downloaded;
extern unsigned int total_bytes_received;
extern int stalled_threads;

void init_pool(pool_t * pool, char * start_url);
void print_pool(pool_t * pool);
bool place(pool_t * pool, char * url);
char * obtain(pool_t * pool);
void * client(void * ptr);
void delete_pool(pool_t * pool);
char * extract_path_from_url(char * url);
char * get_site_dir(char * file_path, char * save_dir);
void write_page_in_file(char * save_dir, char * file_path, char * content);
char * make_file_path(char * link);
char * make_http_get_req(char * file_path, const char * host_str);
char * read_http_get_response(int fd, bool &discon);
void search_for_links(char * content);

int start_search(char ** dirs, unsigned int n_dirs, unsigned int N, int sock_fd, pool_t * pool, pthread_t * clients_tids,
                 char * host, char * cmd);
char ** get_dirs(pool_t * pool, unsigned int & ndirs, char * save_dir);


#endif // MYCRAWLER_H_INCLUDED
