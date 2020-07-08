#ifndef MYHTTPD_H_INCLUDED
#define MYHTTPD_H_INCLUDED

#include "helping_funs.h"
#define POOL_SIZE 100

using namespace std;

typedef struct
{
    int clients_socks[POOL_SIZE];
    int start;
    int end;
    int size;
} pool_t;

extern pthread_mutex_t mtx;
extern pthread_cond_t cond_nonempty;
extern pthread_cond_t cond_nonfull;
extern pool_t pool;

extern unsigned int pages_served;
extern unsigned int total_bytes_sent;

extern pthread_mutex_t mtx2;

void init_pool(pool_t * pool);
void place(pool_t * pool, int client_sock);
int obtain(pool_t * pool);
void * consumer(void * ptr);
char * read_http_get_req(int fd, bool &err);
bool valid_http_get(char * get_req);
char * make_http_get_response(int opt, char * content);


#endif // MYHTTPD_H_INCLUDED
