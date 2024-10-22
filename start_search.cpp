
#include "mycrawler.h"
#include "jobexec.h"
#include "worker.h"

int start_search(char ** dirs, unsigned int n_dirs, unsigned int N, int sock_fd, pool_t * pool, pthread_t * clients_tids,
                 char * host, char * cmd)
{
    pid_t pid;
	unsigned int i;

	/*
	if (N > n_dirs)
	{
        N = n_dirs;
        cout<<"-! WARNING !- There are less directories than workers so N will be set as the number of directories"<<endl;
	    cout<<"-> N (new value): '"<<N<<"'"<<endl;
	}
	*/

	workers_pids_size = N;
    workers_pids = (int *)malloc(N * sizeof(int));

	char ** boss_to_worker_ffs_names = (char **) malloc(N * sizeof(char *));
    char ** worker_to_boss_ffs_names = (char **) malloc(N * sizeof(char *));

	//create and store fifos' names
	for(unsigned int i = 0; i < N; i++)
	{
        char i_str[get_number_of_digits(i) + 1];
        sprintf(i_str, "%d", i);

        unsigned int fifo_name_len = strlen("jobexec_to_worker.fifo") + 1 + get_number_of_digits(i);
        boss_to_worker_ffs_names[i] = (char *) malloc(fifo_name_len * sizeof(char));
        strcpy(boss_to_worker_ffs_names[i], "jobexec_to_worker");
        strcat(boss_to_worker_ffs_names[i], i_str);
        strcat(boss_to_worker_ffs_names[i], ".fifo");

        fifo_name_len = strlen("worker_to_jobexec.fifo") + 1 + get_number_of_digits(i);
        worker_to_boss_ffs_names[i] = (char *) malloc(fifo_name_len * sizeof(char));
        strcpy(worker_to_boss_ffs_names[i], "worker");
        strcat(worker_to_boss_ffs_names[i], i_str);
        strcat(worker_to_boss_ffs_names[i], "_to_jobexec.fifo");
	}

    for (i = 0; i < N; i++)  //N children (workers) will be created with fork
    {
        if (mkfifo(boss_to_worker_ffs_names[i], 0666) == -1)
        {
            perror("problem in making fifo");
            exit(2);
        }
        if (mkfifo(worker_to_boss_ffs_names[i], 0666) == -1)
        {
            perror("problem in making fifo");
            exit(2);
        }

        if ((pid = fork()) == -1)
        {
            perror("fork failed");
            exit(1);
        }
        else if (pid == 0)
        {//child (worker) process
            worker(i, boss_to_worker_ffs_names[i], worker_to_boss_ffs_names[i]);
            delete_str_array(boss_to_worker_ffs_names, N);
            delete_str_array(worker_to_boss_ffs_names, N);
            free(workers_pids);

            delete_pool(pool);
            delete_str_array(dirs, n_dirs);
            free(clients_tids);
            free(host);
            free(cmd);
            exit(0);
        }
        else
        {
            workers_pids[i] = pid; //store children's pids in a global array
        }
    }

    //parent - job executor process
    job_executor(boss_to_worker_ffs_names, worker_to_boss_ffs_names, dirs, N, n_dirs, sock_fd);
    delete_str_array(boss_to_worker_ffs_names, N);
    delete_str_array(worker_to_boss_ffs_names, N);
    free(workers_pids);

    return 0;
}
