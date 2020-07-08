
#include "myhttpd.h"

int main(int argc, char ** argv)
{
    if (argc != 9)
    {
        cout<<"-! ERROR - Wrong number of parameters !-"<<endl;
        exit(1);
    }

    int serving_sock = -1;
    int serving_port = -1;
    int command_sock = -1;
    int command_port = -1;
    int n_threads = -1;
    char * root_dir = NULL;

    char * serving_port_str = get_parameter(argv, argc, "-p");
    if (serving_port_str == NULL)
    {
        cout<<"-! ERROR - Serving port parameter does not exist !-"<<endl;
        exit(1);
    }
    else if (!is_unsigned_int_number(serving_port_str))
    {
        cout<<"-! ERROR - Service port's -p parameter must be an unsigned int !-"<<endl;
        exit(1);
    }
    else
    {
        serving_port = atoi(serving_port_str);
    }

    char * command_port_str = get_parameter(argv, argc, "-c");
    if (command_port_str == NULL)
    {
        cout<<"-! ERROR - Command port parameter does not exist !-"<<endl;
        exit(1);
    }
    else if (!is_unsigned_int_number(command_port_str))
    {
        cout<<"-! ERROR - Command port's -c parameter must be an unsigned int !-"<<endl;
        exit(1);
    }
    else
    {
        command_port = atoi(command_port_str);
    }

    char * n_threads_str = get_parameter(argv, argc, "-t");
    if (n_threads_str == NULL)
    {
        cout<<"-! ERROR - Number of threads parameter does not exist !-"<<endl;
        exit(1);
    }
    else if (!is_unsigned_int_number(n_threads_str))
    {
        cout<<"-! ERROR - Number of threads' -t parameter must be an unsigned int !-"<<endl;
        exit(1);
    }
    else
    {
        n_threads = atoi(n_threads_str);
    }

    root_dir = get_parameter(argv, argc, "-d");
    if (root_dir == NULL)
    {
        cout<<"-! ERROR - Root directory parameter does not exist !-"<<endl;
        exit(1);
    }
    else if (!dir_exists(root_dir))
    {
        cout<<"-! ERROR - Root directory does not exist !-"<<endl;
        exit(1);
    }

    if (command_port == serving_port)
    {
        cout<<"-! ERROR - Command and serving ports must be different !-"<<endl;
        exit(1);
    }

    sep_term_line();
    cout<<"--> Parameters given <--"<<endl;
    cout<<"-> Serving port (-p): "<<serving_port<<endl;
    cout<<"-> Command port (-c): "<<command_port<<endl;
    cout<<"-> Number of threads (-t): "<<n_threads<<endl;
    cout<<"-> Root directory (-d): '"<<root_dir<<"'"<<endl;
    sep_term_line();
    cout<<"--> SERVER IS ON ! <--"<<endl;
    sep_term_line();

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    int enable = 1;
    struct pollfd poll_tids[2];

    //service --------------------------------------------------------------

    if((serving_sock = socket(AF_INET, SOCK_STREAM , 0)) ==  -1)
    {
        perror("Socket  creation  failed!");
        exit(1);
    }

    if (setsockopt(serving_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    poll_tids[0].fd = serving_sock;
    poll_tids[0].events = POLLIN;

    bind_on_port(serving_sock, serving_port);
    if(listen(serving_sock, 128) < 0)
    {
        perror("listen");
        exit(1);
    }

    //command ----------------------------------------------------------

    if((command_sock = socket(AF_INET, SOCK_STREAM , 0)) ==  -1)
    {
        perror("Socket  creation  failed!");
        exit(1);
    }

    if (setsockopt(command_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    poll_tids[1].fd = command_sock;
    poll_tids[1].events = POLLIN;

    bind_on_port(command_sock, command_port);
    if(listen(command_sock , 128) < 0)
    {
        perror("listen");
        exit(1);
    }

    //-----------------------------------------------------------


    init_pool(&pool);
    pthread_mutex_init(&mtx, 0);
    pthread_mutex_init(&mtx2, 0);  //for global variables used in stats

    pthread_cond_init(&cond_nonempty, 0);
    pthread_cond_init(&cond_nonfull, 0);

    pthread_t * cons_tids;
    if ((cons_tids = (pthread_t *) malloc(n_threads * sizeof(pthread_t))) == NULL)
    {
        perror("malloc");
        exit(1);
    }

    for(int i=0; i < n_threads; i++)
    {
        int err = pthread_create(cons_tids + i, NULL, consumer, root_dir);
        if(err < 0)
        {
            perror("pthread_create");
            exit(1);
        }
    }

    while(1)
    {
        if(poll(poll_tids, 2, -1) == -1)
        {
            perror("Eror in poll");
            exit(1);
        }

        if(poll_tids[0].revents == POLLIN)
        {//service sock

            int client_sock = accept(serving_sock, NULL , NULL);
            if (client_sock < 0)
            {
                perror("accept");
                exit(1);
            }

            place(&pool, client_sock);   //push back client's sockets' fds
            pthread_cond_signal(&cond_nonempty);
        }
        else if (poll_tids[0].revents == POLLHUP)
        {
            perror("POLLHUP");
            exit(1);
        }

        if(poll_tids[1].revents == POLLIN)
        {//command sock

            int cmd_sock = accept(command_sock, NULL , NULL);
            if (cmd_sock < 0)
            {
                perror("accept");
                exit(1);
            }

            if (!send_message(cmd_sock, "--> Type 'STATS' to get statistics or 'SHUTDOWN' to stop server.\r\n-> "))
            {
                cout<<"=> WARNING - Disconnection .."<<endl;
            }

            bool discon;
            char * cmd = read_cmd(cmd_sock, discon);
            if (discon == 1)
            {
                cout<<"=> WARNING - Connection to command socket lost. Server could not get a command .."<<endl;
            }
            else
            {
                if(strcmp(cmd, "SHUTDOWN") == 0)
                {
                    free(cmd);
                    for(int i=0; i < n_threads; i++)
                    {
                        place(&pool, -1); //push back -1 to signal threads to finish (all existing clients' requests will be served first)
                        pthread_cond_signal(&cond_nonempty);
                    }
                    close(cmd_sock);
                    break;
                }
                else if(strcmp(cmd, "STATS") == 0)
                {
                    gettimeofday(&t2, NULL);

                    int elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000;      // sec to ms
                    elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000;   // us to ms
                    char * elapsed_time_str = ms_to_time(elapsedTime);

                    pthread_mutex_lock(&mtx2); //global jointly accessed variables must be protected with mutexes
                        char * stats_str = get_stats_str(1, pages_served, total_bytes_sent, elapsed_time_str);
                    pthread_mutex_unlock(&mtx2);

                    free(elapsed_time_str);

                    if (!send_message(cmd_sock, stats_str))
                    {
                        cout<<"=> WARNING - Disconnection. Server could not send stats .."<<endl;
                    }
                    free(stats_str);
                }
                else
                {
                    const char * msg = "-! ERROR - Wrong server command !-\r\n";
                    if (!send_message(cmd_sock, msg))
                    {
                        cout<<"=> WARNING - Disconnection. Server could not send response .."<<endl;
                    }
                }

                free(cmd);
                close(cmd_sock);
            }
        }
        else if (poll_tids[1].revents == POLLHUP)
        {
            perror("POLLHUP");
            exit(1);
        }
    }


    for(int i=0 ; i < n_threads; i++)
    { //wait for all threads to finish
        pthread_join(*(cons_tids + i), 0);
    }

    free(cons_tids);

    pthread_cond_destroy(&cond_nonempty);
    pthread_cond_destroy(&cond_nonfull);
    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&mtx2);

    return 0;
}
