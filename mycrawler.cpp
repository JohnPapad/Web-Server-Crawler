#include "mycrawler.h"


pthread_mutex_t mtx;
pthread_mutex_t mtx2;
pthread_cond_t cond_nonempty;
pool_t pool;

unsigned int pages_downloaded = 0;
unsigned int total_bytes_received = 0;
int stalled_threads = 0; //for checking if crawler has finished downloading webpages from server

void init_pool(pool_t * pool, char * start_url)
{//initialize pool with the starting url given from command line arguments
    pool->used_urls_size = 1;
    pool->curr_urls_size = 1;

    pool->used_urls = (char **) malloc(sizeof(char *));
    pool->used_urls[0] = (char *) malloc((strlen(start_url) + 1) * sizeof(char));
    strcpy(pool->used_urls[0], start_url);

    pool->curr_urls = (char **) malloc(sizeof(char *));
    pool->curr_urls[0] = start_url;
}


void print_pool(pool_t * pool)
{
    cout<<"Remaining urls - size:"<<pool->curr_urls_size<<endl;
    for(int i=0; i < pool->curr_urls_size; i++)
    {
        cout<<"'"<<pool->curr_urls[i]<<"'"<<endl;
    }
    cout<<"-----------------------------------------"<<endl;

    cout<<"All used urls - size:"<<pool->used_urls_size<<endl;
    for(int i=0; i < pool->used_urls_size; i++)
    {
        cout<<"'"<<pool->used_urls[i]<<"'"<<endl;
    }
    cout<<"-----------------------------------------"<<endl;
}


void delete_pool(pool_t * pool)
{
    for(int i=0; i < pool->used_urls_size; i++)
    {
        free(pool->used_urls[i]);
    }

    for(int i=0; i < pool->curr_urls_size; i++)
    {
        free(pool->curr_urls[i]);
    }
    free(pool->curr_urls);
    free(pool->used_urls);
}


bool place(pool_t * pool, char * url)
{
    pthread_mutex_lock(&mtx);

    bool url_placed = 0;
    if(url != NULL)
    {
        bool duplicate_found = 0;
        for(int i=0; i < pool->used_urls_size; i++)
        {// check if webpage has already been requested and served from server
            if (strcmp(pool->used_urls[i], url) == 0)
            {
                duplicate_found = 1;
                break;
            }
        }

        if (!duplicate_found)
        {
            pool->curr_urls_size++;
            pool->used_urls_size++;

            pool->used_urls = (char **) realloc(pool->used_urls, pool->used_urls_size * sizeof(char *));
            pool->used_urls[pool->used_urls_size -1] = (char *) malloc((strlen(url) + 1) * sizeof(char));
            strcpy(pool->used_urls[pool->used_urls_size -1], url);

            pool->curr_urls = (char **) realloc(pool->curr_urls, pool->curr_urls_size * sizeof(char *));
            pool->curr_urls[pool->curr_urls_size - 1] = url;

            url_placed = 1;
        }
        else
        {
            free(url);
        }
    }
    else
    {//signal for thread to finish
        pool->curr_urls_size++;
        pool->curr_urls = (char **) realloc(pool->curr_urls, pool->curr_urls_size * sizeof(char *));
        pool->curr_urls[pool->curr_urls_size - 1] = NULL;
        url_placed = 1;
    }

    pthread_mutex_unlock(&mtx);
    return url_placed;
}


char * obtain(pool_t * pool)
{
    pthread_mutex_lock(&mtx);

    stalled_threads++;
    while(pool->curr_urls_size <= 0)
    {
        pthread_cond_wait(&cond_nonempty, &mtx);
    }
    stalled_threads--;

    char * url = pool->curr_urls[pool->curr_urls_size -1];
    pool->curr_urls_size--;
    pool->curr_urls = (char **) realloc(pool->curr_urls, pool->curr_urls_size * sizeof(char *));

    pthread_mutex_unlock(&mtx);
    return url;
}


void * client(void * ptr)
{
    client_arg * cl_arg = (client_arg *) ptr;
    struct sockaddr * serverptr = cl_arg->serverptr;
    int server_size = cl_arg->server_size;
    char * host_str = cl_arg->host_str;
    char * save_dir = cl_arg->save_dir;

    while(1)
    {
        int server_sock;
        //print_pool(&pool);
        char * file_path = obtain(&pool);
        if (file_path == NULL)
        {
            break;
        }

        if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) ==  -1)
        {
            perror("Socket  creation  failed!");
            exit(1);
        }

        int enable = 1;
        if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
        {
            perror("setsockopt(SO_REUSEADDR) failed");
            exit(1);
        }

        if(connect(server_sock, serverptr, server_size) < 0)
        {
            cout<<"["<<syscall(SYS_gettid)<<"]";
            perror("could not connect");
            exit(1);
        }

        char * get_req = make_http_get_req(file_path, host_str);
        //cout<<"get_req: '"<<get_req<<"'"<<endl;
        bool err = send_message(server_sock, get_req);
        free(get_req);
        if (err == 0)
        {
            cout<<"["<<syscall(SYS_gettid)<<"]"<<"-> WARNING - Lost connection to server. Crawler could not send http get request .."<<endl;
        }
        else
        {
            bool discon;
            char * answer = NULL;
            answer = read_http_get_response(server_sock, discon);
            if (discon == 1)
            {
                free(answer);
                cout<<"["<<syscall(SYS_gettid)<<"]"<<"-! ERROR - Lost connection to server. Crawler could not receive answer from server !-"<<endl;
                exit(1);
            }
            else
            {
                if (answer != NULL)
                {
                    //cout<<"get_req_answer: '"<<answer<<"'"<<endl;
                    write_page_in_file(save_dir, file_path, answer);

                    if ((strcmp(answer, "<html>Sorry dude, could not find this file.</html>") == 0)
                         || (strcmp(answer, "<html>Trying to access this file but do not think I can make it.</html>") == 0)
                         || (strcmp(answer, "<html>Http get request is not valid.</html>") == 0))
                    {
                        //cout<<"-> '"<<answer<<"'"<<endl;
                    }
                    else
                    {
                        pthread_mutex_lock(&mtx2);
                            pages_downloaded++;
                            total_bytes_received += strlen(answer);
                        pthread_mutex_unlock(&mtx2);

                        search_for_links(answer);
                    }
                    free(answer);
                }
                else
                {
                    cout<<"-> WARNING - Http get response is not valid. It will be ignored .."<<endl;
                }
            }
        }

        free(file_path);
        close(server_sock);
        //cout<<"====================================================="<<endl;
    }

    return NULL;
}


char * make_http_get_req(char * file_path, const char * host_str)
{
    const char * get_str = "GET ";
    const char * get_req_rest = " HTTP/1.1\r\nUser-Agent: mycrawler/1.0.0 (X11; Ubuntu; Linux x86_64; rv:60.0)\r\nHost: ";
    const char * get_req_rest2 = "\r\nAccept-Language: en-us\r\nAccept-Encoding: gzip, deflate\r\nConnection: Closed\r\n\r\n";
    char * get_req = (char *) malloc((strlen(get_str) + strlen(file_path) + strlen(host_str) + strlen(get_req_rest) + strlen(get_req_rest2) + 1) * sizeof(char));
    strcpy(get_req, get_str);
    strcat(get_req, file_path);
    strcat(get_req, get_req_rest);
    strcat(get_req, host_str);
    strcat(get_req, get_req_rest2);

    return get_req;
}


char * read_http_get_response(int fd, bool &err)
{
    int content_size = -1;
    bool prev_cr = 0;
    bool prev_blr = 0;
    bool next_is_size = 0;
    err = 0;
    char * word = (char * ) malloc(sizeof(char));
    unsigned int word_size = 1;
    unsigned int word_counter = 0;

    char buf[1];
    while(1)
    { //read header first
        int bytes_read = read(fd, buf, 1);
        if (bytes_read <= 0)
        {
            free(word);
            err = 1;
            return NULL;
        }

        // checking header for validation
        if ((prev_blr == 0) && (buf[0] == '\n')) { free(word); return NULL; }
        else if ((prev_blr == 1) && (buf[0] != '\n')) { free(word); return NULL; }

        if(buf[0] == '\r')
        {
            prev_blr = 1;
            continue;
        }
        else
        {
            prev_blr = 0;
        }

        if(buf[0] == '\n')
        {
            if (prev_cr == 1)
            {//reading header until two consecutive LFs
                break;
            }
            prev_cr = 1;
        }
        else
        {
            prev_cr = 0;
        }


        if ((buf[0] == ' ') || (buf[0] == '\n'))
        {//formating words looking for content length field
            word_counter++;
            word[word_size -1] = '\0';
            if (next_is_size == 1)
            {
                next_is_size = 0;
                content_size = atoi(word);
            }
            else if ((word_counter == 1) && (strcmp(word, "HTTP/1.1") != 0))
            {
                free(word);
                return NULL;
            }
            else if (strcmp(word, "Content-Length:") == 0)
            {
                next_is_size = 1;
            }

            free(word);
            word = (char * ) malloc(sizeof(char));
            word_size = 1;
        }
        else
        {
            word[word_size -1] = buf[0];
            word_size++;
            word = (char *) realloc(word, word_size * sizeof(char));
        }
    }
    free(word);

    if (content_size == -1) { return NULL; }

    char * content = (char *) calloc((content_size + 1), sizeof(char));
    int read_size = 4048;
    int content_counter = 0;
    while(1)
    {//read content (webpage) in chunks
        char * buf = (char *) calloc((read_size + 1), sizeof(char));

        int bytes_read = read(fd, buf, read_size);
        content_counter += bytes_read;

        if (bytes_read <= 0)
        {
            free(content);
            free(buf);
            err = 1;
            return NULL;
        }

        strncat(content, buf, bytes_read);

        free(buf);
        if (content_counter == content_size)
        {
            break;
        }
    }

    content[content_size] = '\0';
    return content;
}


void write_page_in_file(char * save_dir, char * file_path, char * content)
{//create webpage in save directory
    char * site_dir = get_site_dir(file_path, save_dir);
    if(!dir_exists(site_dir))
    {//if dir does not exist create it
        mkdir(site_dir, 0777);
    }
    free(site_dir);

    char * page_path = (char *) malloc((strlen(save_dir) + strlen(file_path) + 1) * sizeof(char));
    strcpy(page_path, save_dir);
    strcat(page_path, file_path);

    ofstream outfile (page_path, ios_base::app | ios_base::out);
    outfile<<content;
    free(page_path);
}


char * extract_path_from_url(char * url)
{//extracting file path from starting url
    char * rest = url;
    char * file_path = strtok_r(rest, "//", &rest);
    file_path = strtok_r(rest, "/", &rest);
    file_path = strtok_r(rest, "", &rest);

    char * new_file_path = (char * ) malloc((strlen(file_path) + 2) * sizeof(char));
    strcpy(new_file_path, "/");
    strcat(new_file_path, file_path);

    return new_file_path;
}


char * get_site_dir(char * file_path, char * save_dir)
{
    char * file_path_cpy = (char *) malloc((strlen(file_path) + 1) * sizeof(char));
    strcpy(file_path_cpy, file_path);
    char * rest = file_path_cpy;
    char * site = strtok_r(rest, "/", &rest);
    char * site_dir = (char *) malloc((strlen(save_dir) + strlen(site) + 2) * sizeof(char));
    strcpy(site_dir, save_dir);
    strcat(site_dir, "/");
    strcat(site_dir, site);
    free(file_path_cpy);

    return site_dir;
}


char * make_file_path(char * link)
{
    if (link[0] == '.')
    {
        char * file_path = (char * ) malloc((strlen(link) - 2 + 1) * sizeof(char));
        strncpy(file_path, link + 2, strlen(link) - 2 );
        file_path[strlen(link) - 2 ] = '\0';
        return file_path;
    }
    else
    {
        int end_i=0;
        for(unsigned int i=4; ;i++)
        {
            if (link[i] == '_')
            {
                end_i = i;
                break;
            }
        }

        char site_id[end_i - 4 + 1];
        strncpy(site_id, link + 4, end_i - 4);
        site_id[end_i - 4] = '\0';

        const char * site_str = "/site";
        char * file_path = (char * ) malloc((strlen(site_id) + strlen(site_str) + strlen(link) + 2) * sizeof(char));
        strcpy(file_path, site_str);
        strcat(file_path, site_id);
        strcat(file_path, "/");
        strcat(file_path, link);

        return file_path;
    }
}


void search_for_links(char * content)
{//searching webpage for links
    char * rest = content;
    char * word = NULL;

    while ((word = strtok_r(rest, " \t", &rest)))
    { //for each webpage's word
        if (strcmp(word, "<a") == 0)
        {//found link opening tag
            word = strtok_r(rest, " \t", &rest);  //link will be the next word

            char * link = NULL;
            int start_i = -1;
            int end_i = -1;
            unsigned int i = 0;

            //finding at which indexes the url starts and ends
            while(1)
            {
                if (word[i] == '"')
                {
                    if (start_i == -1)
                    {
                        start_i = i;
                    }
                    else
                    {
                        end_i = i;
                        break;
                    }
                }
                i++;
            }

            link = (char *) malloc(((end_i - start_i)) * sizeof(char));
            strncpy(link, word + (start_i+1), end_i - start_i );
            link[end_i - start_i -1] = '\0';
            char * file_path = make_file_path(link);
            free(link);

            if (place(&pool, file_path))
            {
                pthread_cond_signal(&cond_nonempty);
            }
        }
    }
}


char ** get_dirs(pool_t * pool, unsigned int & ndirs, char * save_dir)
{// getting all websites' directories (no duplicates)
    char ** dirs = NULL;
    ndirs = 0;

    for(int i = 0; i < pool->used_urls_size; i++)
    {
        bool already_exists = 0;
        char * site_dir = get_site_dir(pool->used_urls[i], save_dir);
        for(unsigned int j = 0; j < ndirs; j++)
        {
            if(strcmp(site_dir, dirs[j]) == 0)
            {
                already_exists = 1;
                break;
            }
        }

        if(!already_exists)
        {
            dirs = (char **)realloc(dirs, (ndirs + 1) * sizeof(char *));
            dirs[ndirs] = site_dir;
            ndirs++;
        }
        else
        {
            free(site_dir);
        }
    }

    return dirs;
}

