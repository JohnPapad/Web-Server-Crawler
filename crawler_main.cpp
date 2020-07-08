
#include "mycrawler.h"

int main(int argc, char ** argv)
{
    if (argc != 12)
    {
        cout<<"-! ERROR - Wrong number of parameters !-"<<endl;
        exit(1);
    }

    int server_port = -1;
    int command_sock = -1;
    int command_port = -1;
    int n_threads = -1;
    char * save_dir = NULL;
    struct hostent * host;

    char * host_or_ip_str = get_parameter(argv, argc-1, "-h");
    if (host_or_ip_str == NULL)
    {
        cout<<"-! ERROR - Host or IP parameter does not exist !-"<<endl;
        exit(1);
    }
    else if (is_unsigned_int_number(host_or_ip_str))
    {
        cout<<"-! ERROR -h parameter must be host's IP address or name !-"<<endl;
        exit(1);
    }
    else
    {
        if(is_IP(host_or_ip_str))
        {
            struct in_addr host_address;
            inet_aton(host_or_ip_str, &host_address);
            host = gethostbyaddr((const char*) & host_address, sizeof(host_address), AF_INET);
            if(host == NULL)
            {
                printf("IP-address :%s could  not be  resolved\n", host_or_ip_str);
                exit (1);
            }
        }
        else
        {
            if( (host = gethostbyname(host_or_ip_str)) == NULL )
            {
                printf("Could not resolved Name: %s\n", host_or_ip_str);
                exit(1);
            }
        }
    }

    char * server_port_str = get_parameter(argv, argc-1, "-p");
    if (server_port_str == NULL)
    {
        cout<<"-! ERROR - Server port parameter does not exist !-"<<endl;
        exit(1);
    }
    else if (!is_unsigned_int_number(server_port_str))
    {
        cout<<"-! ERROR - Server port's -p parameter must be an unsigned int !-"<<endl;
        exit(1);
    }
    else
    {
        server_port = atoi(server_port_str);
    }

    char * command_port_str = get_parameter(argv, argc-1, "-c");
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

    char * n_threads_str = get_parameter(argv, argc-1, "-t");
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

    save_dir = get_parameter(argv, argc-1, "-d");
    if (save_dir == NULL)
    {
        cout<<"-! ERROR - Save directory parameter does not exist !-"<<endl;
        exit(1);
    }
    else if (!dir_exists(save_dir))
    {
        cout<<"-! ERROR - Save directory does not exist !-"<<endl;
        exit(1);
    }

    //checking if save directory is empty
    char sys_cmd[1024];
    snprintf(sys_cmd, 1024, "test $(ls -A \"%s\" 2>/dev/null | wc -l) -ne 0", save_dir);
    int status = system(sys_cmd);
    int exitcode = WEXITSTATUS(status);

    if (exitcode != 1)
    {
        cout<<"-> WARNING - The saving directory is not empty. It will be purged"<<endl;
        char sys_cmd[1024];
        snprintf(sys_cmd, 1024, "exec rm -r %s/*", save_dir);
        system(sys_cmd);
    }

    if (command_port == server_port)
    {
        cout<<"-! ERROR - Command and server ports must be different !-"<<endl;
        exit(1);
    }

    char * starting_url = argv[argc - 1];

    client_arg cl_arg;
    cl_arg.save_dir = save_dir;
    cl_arg.host_str = (char *) malloc((strlen(host_or_ip_str) + strlen(server_port_str) + 2) * sizeof(char));
    strcpy(cl_arg.host_str, host_or_ip_str);
    strcat(cl_arg.host_str, ":");
    strcat(cl_arg.host_str, server_port_str);

    sep_term_line();
    cout<<"--> Parameters given <--"<<endl;
    cout<<"-> Host name or IP (-h): '"<<host_or_ip_str<<"'"<<endl;
    cout<<"-> Host: '"<<cl_arg.host_str<<"'"<<endl;
    cout<<"-> Server port (-p): "<<server_port<<endl;
    cout<<"-> Command port (-c): "<<command_port<<endl;
    cout<<"-> Number of threads (-t): "<<n_threads<<endl;
    cout<<"-> Save directory (-d): '"<<save_dir<<"'"<<endl;
    cout<<"-> Starting URL: '"<<starting_url<<"'"<<endl;
    sep_term_line();
    cout<<"--> CRAWLER IS ON ! <--"<<endl;
    sep_term_line();

    starting_url = extract_path_from_url(starting_url);

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    //command ----------------------------------------------------------

    if((command_sock = socket(AF_INET, SOCK_STREAM , 0)) ==  -1)
    {
        perror("Socket  creation  failed!");
        exit(1);
    }

    int enable = 1;
    if (setsockopt(command_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(1);
    }

    bind_on_port(command_sock, command_port);
    if(listen(command_sock , 128) < 0)
    {
        perror("listen");
        exit(1);
    }

    //---------------------------------------------------------------

    struct sockaddr_in server;
    struct sockaddr * serverptr = (struct sockaddr *) &server;

    server.sin_family = AF_INET;
    memcpy(&server.sin_addr, host->h_addr, host->h_length);
    server.sin_port = htons(server_port);

    cl_arg.serverptr = serverptr;
    cl_arg.server_size = sizeof(server);

    init_pool(&pool, starting_url);
    pthread_mutex_init(&mtx, 0);
    pthread_mutex_init(&mtx2, 0);
    pthread_cond_init(&cond_nonempty, 0);

    pthread_t * clients_tids;
    if ((clients_tids = (pthread_t *) malloc(n_threads * sizeof(pthread_t))) == NULL)
    {
        perror("malloc");
        exit(1);
    }

    for(int i=0; i < n_threads; i++)
    {
        int err = pthread_create(clients_tids + i, NULL, client, &cl_arg);
        if(err < 0)
        {
            perror("pthread_create");
            exit(1);
        }
    }

    while(1)
    { // command
        int cmd_sock = accept(command_sock, NULL , NULL);
        if (cmd_sock < 0)
        {
            perror("accept");
            exit(1);
        }

        if (!send_message(cmd_sock, "--> Type 'SEARCH' to enter searching queries, 'STATS' to get statistics or 'SHUTDOWN' to exit crawler.\r\n-> "))
        {
            cout<<"=> WARNING - Disconnection .."<<endl;
        }

        bool err;
        char * cmd = read_cmd(cmd_sock, err);
        if (err == 1)
        {
            cout<<"=> WARNING - Connection to command socket lost. Crawler could not get a command .."<<endl;
        }
        else
        {
            if(strcmp(cmd, "SHUTDOWN") == 0)
            {
                free(cmd);
                for(int i=0; i < n_threads; i++)
                {
                    place(&pool, NULL);
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

                pthread_mutex_lock(&mtx2);
                    char * stats_str = get_stats_str(0, pages_downloaded, total_bytes_received, elapsed_time_str);
                pthread_mutex_unlock(&mtx2);

                free(elapsed_time_str);

                if (!send_message(cmd_sock, stats_str))
                {
                    cout<<"=> WARNING - Disconnection. Crawler could not send stats .."<<endl;
                }
                free(stats_str);
            }
            else
            {
                if (strcmp("SEARCH", cmd) == 0)
                {
                    if (!send_message(cmd_sock, "--> Please wait.. \r\n"))
                    {
                        cout<<"=> WARNING - Disconnection .."<<endl;
                    }
                    pthread_mutex_lock(&mtx);
                        if ((stalled_threads == n_threads) && (pool.curr_urls_size == 0))
                        {
                            //print_pool(&pool);
                            unsigned int ndirs;
                            char ** dirs = get_dirs(&pool, ndirs, save_dir);
                            start_search(dirs, ndirs, 5, cmd_sock, &pool, clients_tids, cl_arg.host_str, cmd);
                            delete_str_array(dirs, ndirs);
                        }
                        else
                        {
                            const char * msg = "=> WARNING - Crawler has not finished downloading pages from server yet. Please try later ..\r\n";
                            if (!send_message(cmd_sock, msg))
                            {
                                cout<<"=> WARNING - Disconnection .."<<endl;
                            }
                        }
                    pthread_mutex_unlock(&mtx);
                }
                else
                {
                    const char * msg = "-! ERROR - Wrong server command !-\r\n";
                    if (!send_message(cmd_sock, msg))
                    {
                        cout<<"=> WARNING - Disconnection. Crawler could not send response .."<<endl;
                    }
                }
            }

            free(cmd);
            close(cmd_sock);
        }
    }

    for(int i=0 ; i < n_threads; i++)
    {
        pthread_join(*(clients_tids + i), 0);
    }

    free(clients_tids);
    free(cl_arg.host_str);
    delete_pool(&pool);

    pthread_cond_destroy(&cond_nonempty);
    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&mtx2);

    return 0;
}
