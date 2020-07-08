#include "myhttpd.h"

pthread_mutex_t mtx;
pthread_mutex_t mtx2;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;
pool_t pool;

unsigned int pages_served = 0;
unsigned int total_bytes_sent = 0;


//code from class's notes has been used
void init_pool(pool_t * pool)
{
    pool->start = 0;
    pool->end = -1;
    pool->size = 0;
}

void place(pool_t * pool, int client_sock)
{
    pthread_mutex_lock(&mtx);
    while(pool->size >= POOL_SIZE)
    {
        pthread_cond_wait(&cond_nonfull, &mtx);
    }

    pool->end = (pool->end + 1) % POOL_SIZE;
    pool->clients_socks[pool->end] = client_sock;
    pool->size++;
    pthread_mutex_unlock(&mtx);
}

int obtain(pool_t * pool)
{
    int client_sock = 0;
    pthread_mutex_lock(&mtx);
    while(pool->size <= 0)
    {
        pthread_cond_wait(&cond_nonempty, &mtx);
    }
    client_sock = pool->clients_socks[pool->start];
    pool->start = (pool->start + 1) % POOL_SIZE;
    pool->size--;
    pthread_mutex_unlock(&mtx);
    return client_sock;
}


void * consumer(void * ptr)
{
    char * root_dir = (char *) ptr;
    while(1)
    {
        int client_sock = obtain(&pool);
        pthread_cond_signal(&cond_nonfull);

        if (client_sock == -1)
        { //signal from main thread to finish
            break;
        }

        bool err;
        char * http_get_req = read_http_get_req(client_sock, err);
        if (err == 1)
        {
            cout<<"=> WARNING - Client disconnected. Server could not receive request from client .."<<endl;
        }
        else
        {
                char * answer = NULL;
                unsigned int bytes_sent = 0;
                bool page_served = 0;

                if (valid_http_get(http_get_req) == 1)
                {//if http get's header is ok
                    //cout<<"http get req: '"<<http_get_req<<"'"<<endl;

                    char * rest = http_get_req;
                    char * file_path = strtok_r(rest, " \t", &rest);
                    file_path = strtok_r(rest, " \t", &rest); //extract file path from client's get request
                    char * file_full_path = (char *)malloc((strlen(root_dir) + strlen(file_path) +1 ) * sizeof(char));
                    strcpy(file_full_path, root_dir);
                    strcat(file_full_path, file_path);

                    FILE* file;
                    file = fopen(file_full_path, "r");
                    free(file_full_path);

                    if (file == NULL)
                    {
                        if (errno == ENOENT)
                        {//file does not exist
                            answer = make_http_get_response(1, NULL);
                        }
                        else if (errno == EACCES)
                        {//dont have read rights on the file
                            answer = make_http_get_response(2, NULL);
                        }
                    }
                    else
                    {
                        char * file_content = parse_file(file);
                        answer = make_http_get_response(0, file_content);

                        bytes_sent = strlen(file_content);
                        page_served = 1;
                        free(file_content);
                        fclose(file);
                    }
                }
                else
                {//if http get's header is not ok
                    answer = make_http_get_response(3, NULL);
                    cout<<"=> WARNING - Http get request is not valid .."<<endl;
                }

                if (!send_message(client_sock, answer))
                {
                    cout<<"=> WARNING - Client disconnected. Server could not send response .."<<endl;
                }
                else
                {//requested web page has beem successfully served
                    pthread_mutex_lock(&mtx2);
                        pages_served += page_served;   //update stats
                        total_bytes_sent += bytes_sent;
                    pthread_mutex_unlock(&mtx2);
                }

                free(answer);
                free(http_get_req);
        }

        close(client_sock);
        //cout<<"----------------------------------------"<<endl;
    }

    return NULL;
}


char * read_http_get_req(int fd, bool &err)
{
    err = 0;
    char * req = NULL;
    int total_bytes = 0;
    int chunk_size = 1024;

    while(1)
    {//read get request in chunks
        char * chunk_buf = (char *) calloc(chunk_size, sizeof(char));
        int bytes_read = read(fd, chunk_buf, chunk_size);

        if (bytes_read <= 0)
        {
            free(chunk_buf);
            free(req);
            if (bytes_read < 0)
            {
                perror("problem in reading from socket");
                exit(2);
            }
            err = 1;
            return NULL;
        }

        bool exit_flag = 0;
        bool nonvalid = 0;
        int start_i_req = -1;
        int end_i_req = -1;
        int start_i_chunk = -1;
        int end_i_chunk = -1;
        if (req != NULL)
        {//not first chunk read
            if (bytes_read < 4) //we must always check 4 last chunk's bytes to detect two consecutive LFs
            {//if this chunk is less than 4 bytes we have to look up in previous data too
                start_i_chunk = 0;

                start_i_req = total_bytes - (4 - bytes_read);
                end_i_req = total_bytes;
            }
            else
            {
                start_i_chunk = bytes_read - 4;
            }
            end_i_chunk = bytes_read;
        }
        else
        {//first chunk read
            start_i_chunk = bytes_read - 4;
            end_i_chunk = bytes_read;
        }

        int k = 0;
        int i = -1;
        int req_i = start_i_req;
        int chunk_i = start_i_chunk;
        bool prev_r = 0;      //indicating previous characters found
        bool prev_n = 0;
        bool prev_rn = 0;
        bool prev_rnr = 0;

        while(1)
        {
            char test_char;
            if (k == 0)
            {//first looking up in previous data (if necessary)
                if (req_i < end_i_req)
                {
                    test_char = req[req_i];
                    req_i++;
                    i++;
                }
                else
                {
                    k = 1;
                    continue;
                }
            }
            else if (k == 1)
            {//then in current chunk read
                if (chunk_i < end_i_chunk)
                {
                    test_char = chunk_buf[chunk_i];
                    chunk_i++;
                    i++;
                }
                else
                {
                    k = 2;
                    continue;
                }
            }
            else
            {
                break;
            }

            //apart from trying to detect two consecutive LFs we perform a basic syntax check in request regarding the \r, \n
                if (prev_rnr == 1)
                {
                    if (test_char == '\n')
                    {
                        exit_flag = 1;
                        break;
                    }

                    nonvalid = 1;
                    break;
                }
                else if (prev_rn == 1)
                {
                    if (test_char == '\n')
                    {
                        nonvalid = 1;
                        break;
                    }

                    if (test_char == '\r')
                    {
                        prev_r = 0;
                        prev_n = 0;
                        prev_rn = 0;
                        prev_rnr = 1;
                        continue;
                    }

                    prev_r = 0;
                    prev_n = 0;
                    prev_rn = 0;
                    prev_rnr = 0;
                    continue;
                }
                else if (prev_r == 1)
                {
                    if (test_char == '\n')
                    {
                        prev_r = 0;
                        prev_n = 0;
                        prev_rn = 1;
                        prev_rnr = 0;
                        continue;
                    }

                    nonvalid = 1;
                    break;
                }
                else if (prev_n == 1)
                {
                    if (test_char == '\n')
                    {
                        nonvalid = 1;
                        break;
                    }

                    if (test_char == '\r')
                    {
                        nonvalid = 1;
                        break;
                    }

                    prev_r = 0;
                    prev_n = 0;
                    prev_rn = 0;
                    prev_rnr = 0;
                    continue;
                }
                else if (test_char == '\n')
                {
                    prev_r = 0;
                    prev_n = 1;
                    prev_rn = 0;
                    prev_rnr = 0;
                    continue;
                }
                else if (test_char == '\r')
                {
                    prev_r = 1;
                    prev_n = 0;
                    prev_rn = 0;
                    prev_rnr = 0;
                    continue;
                }
                else
                {
                    prev_r = 0;
                    prev_n = 0;
                    prev_rn = 0;
                    prev_rnr = 0;
                    continue;
                }
        }

        if (nonvalid)
        {
            free(chunk_buf);
            free(req);
            return NULL;
        }

        total_bytes += bytes_read;
        if (req == NULL)
        {//first chunk read
            req = (char *) malloc((bytes_read + 1)* sizeof(char));
            strncpy(req, chunk_buf, bytes_read);
            req[bytes_read] = '\0';
        }
        else
        {
            req = (char *) realloc(req, (total_bytes+1) * sizeof(char));
            strncat(req, chunk_buf, bytes_read);
        }

        free(chunk_buf);
        if (exit_flag)
        {
            break;
        }
    }

    req = (char *) realloc(req, (total_bytes +1) * sizeof(char));
    req[total_bytes] = '\0';  // add \0 at the end of the string

    return req;
}


bool valid_http_get(char * get_req)
{//perform a basic http get validation
    if (get_req == NULL)
    {//error has been detecting from reading the request
        return 0;
    }

    //make sure it is a get request
    if (strlen(get_req) < 5) { return 0; }
    if (get_req[0] != 'G') { return 0; }
    if (get_req[1] != 'E') { return 0; }
    if (get_req[2] != 'T') { return 0; }
    if (get_req[3] != ' ') { return 0; }

    char * get_req_cpy = (char *) malloc((strlen(get_req) + 1) * sizeof(char));
    strcpy(get_req_cpy, get_req);
    unsigned int get_req_nlines;
    char ** get_req_words = strip_string_into_lines(get_req_cpy, get_req_nlines);
    free(get_req_cpy);

    unsigned int host_counter = 0;
    for(unsigned int i=0; i < get_req_nlines; i++)
    {//checking if there is keyword 'Host'
        if(strstr(get_req_words[i], "Host: ") != NULL)
        {
            host_counter++;
            if (i == 0)
            {
                delete_str_array(get_req_words, get_req_nlines);
                return 0;
            }
        }
    }

    delete_str_array(get_req_words, get_req_nlines);
    if(host_counter != 1)
    {//we must not have two of more host fields in http get request
        return 0;
    }
    else
    {
        return 1;
    }
}


char * make_http_get_response(int opt, char * content)
{
    const char * ans_p1;
    char * ans_p2 = get_http_timestamp();
    const char * ans_p3 = "\r\nServer: myhttpd/1.0.0 (Ubuntu64)\r\nContent-Length: ";
    const char * ans_p5 = "\r\nContent-Type: text/html\r\nConnection: Closed\r\n\r\n";
    const char * ans_p6;

    if(opt == 0)
    {
        ans_p1 = "HTTP/1.1 200 OK\r\nDate: ";
        ans_p6 = content;
    }
    else if(opt == 1)
    {
        ans_p1 = "HTTP/1.1 404 Not Found\r\nDate: ";
        ans_p6 = "<html>Sorry dude, could not find this file.</html>";
    }
    else if(opt == 2)
    {
        ans_p1 = "HTTP/1.1 403 Forbidden\r\nDate: ";
        ans_p6 = "<html>Trying to access this file but do not think I can make it.</html>";
    }
    else if(opt == 3)
    {
        ans_p1 = "HTTP/1.1 400 Bad Request\r\nDate: ";
        ans_p6 = "<html>Http get request is not valid.</html>";
    }

    char ans_p4[get_number_of_digits(strlen(ans_p6)) + 1];
    sprintf(ans_p4, "%ld", strlen(ans_p6));
    char * answer = (char *)malloc((strlen(ans_p1) + strlen(ans_p2) + strlen(ans_p3) + strlen(ans_p4) + strlen(ans_p5) + strlen(ans_p6) + 1) * sizeof(char));

    strcpy(answer, ans_p1);
    strcat(answer, ans_p2);
    strcat(answer, ans_p3);
    strcat(answer, ans_p4);
    strcat(answer, ans_p5);
    strcat(answer, ans_p6);

    free(ans_p2);
    return answer;
}
