#include "helping_funs.h"


char * read_cmd(int fd, bool &err)
{
    char * str = (char *) malloc(sizeof(char));
    int str_len = 1;
    int i = 0;
    err = 0;

    char buf[1];
    while(1)
    {
        int bytes_read = read(fd, buf, 1);
        if (bytes_read <= 0)
        {
            free(str);
            if (bytes_read < 0)
            {
                perror("problem in reading from socket");
                exit(3);
            }
            err = 1;
            return NULL;
        }

        if(buf[0] == '\r')
        {
            continue;
        }

        if(buf[0] == '\n')
        {
            break;
        }

        str[i] = buf[0];
        i++;

        if(i == str_len)
        {
            str_len++;
            str = (char *) realloc(str, str_len * sizeof(char));
        }
    }

    str[i] = '\0';  // add \0 at the end of the string
    return str;
}


bool send_message(int fd, const char * response)
{
    int sent_bytes = write(fd, response, strlen(response));
    if (sent_bytes < 0)
    {
        perror("problem in writing in socket");
        exit(3);
    }

    if (sent_bytes == 0)
    {
        return 0;
    }

    if(sent_bytes != (int)strlen(response))
    {
        return 0;
    }

    return 1;
}


char * get_parameter(char ** argv, int argc, const char * par_flag)
{
    for(int i=1; i < argc; i++)
    {
        if(strcmp(argv[i], par_flag) == 0)
        {
            if((i+1) < argc)
            {
                return argv[i+1];
            }
            else
            {
                return NULL;
            }
        }
    }

    return NULL;
}


bool is_IP(const char *str)
{
    unsigned int dot_counter = 0;
    for(unsigned int i=0; i < strlen(str); i++)
    { //iterating the whole string character by character

        if(str[i] == '.')
        {
            dot_counter++;
            if((i == 0) || (i == strlen(str) - 1))
            {
                return 0;
            }
            else
            {
                if(dot_counter == 4)
                {
                    return 0;
                }
            }
        }
        else if(!isdigit(str[i]))
        {
            return 0;
        }
    }

    if(dot_counter == 3)
    {
        return 1;
    }

    return 0;
}

bool is_unsigned_int_number(const char * str)
{
    for(unsigned int i=0; i < strlen(str); i++)
    { //iterating the whole string character by character
        if(!isdigit(str[i]))
        {// and checking if every character is a number
            return 0;
        }
    }

    return 1;
}

void sep_term_line()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    unsigned int width = w.ws_col;
    for(unsigned int i = 0; i < width; i++)
    {
        cout<<"-";
    }
    cout<<endl;
}

int dir_exists(char * in_dir)
{//checking that text files at every dir exist
    DIR * FD;

    /* Scanning directory */
    if (NULL == (FD = opendir(in_dir)))
    {
        //fprintf(stderr, "-! Error : Failed to open input directory - %s !-\n", strerror(errno));
        closedir(FD);
        return 0;
    }
    else
    {
        closedir(FD);
        return 1;
    }
}

FILE * open_file(char * fn)
{
    FILE* random_file;
    random_file = fopen(fn, "r");

    if (!random_file)
    {
        fprintf(stderr, "-! ERROR - docfile does not exist !-\n");
        return NULL;
    }

    return random_file;
}


char ** strip_string_into_lines(char * str, unsigned int & n_lines)
{//returns a char * array which contains each cmd's word
    char ** lines = NULL;
    char * rest = str;
    char * line = NULL;
    unsigned int lines_counter = 0;

    while ((line = strtok_r(rest, "\n", &rest)))
    {
        lines = (char **) realloc(lines, (lines_counter + 1) * sizeof(char *));
        lines[lines_counter] = (char *) malloc((strlen(line) + 1) * sizeof(char));  // plus one for \0
        strcpy(lines[lines_counter], line);
        lines_counter++;
    }

    n_lines = lines_counter;
    return lines;
}


char ** strip_line_into_words(char * cmd, unsigned int & cmd_nwords)
{//returns a char * array which contains each cmd's word
    char * cmd_cpy = (char *) malloc((strlen(cmd) + 1) * sizeof(char));
    strcpy(cmd_cpy, cmd);

    char ** cmd_words = NULL;
    char * rest = cmd_cpy;
    char * word = NULL;
    unsigned int word_counter = 0;

    while ((word = strtok_r(rest, " \t", &rest)))
    { //for each cmd's word
        cmd_words = (char **) realloc(cmd_words, (word_counter + 1) * sizeof(char *));
        cmd_words[word_counter] = (char *) malloc((strlen(word) + 1) * sizeof(char));  // plus one for \0
        strcpy(cmd_words[word_counter], word);
        word_counter++;
    }

    free(cmd_cpy);
    cmd_nwords = word_counter;
    return cmd_words;
}

void delete_str_array(char ** str_array, unsigned int array_size)
{
    for(unsigned int i = 0; i < array_size; i++)
    {
        free(str_array[i]);
    }

    free(str_array);
}


char * parse_file(FILE * fp)
{// storing docfile's lines into docs array - checking docfile for errors
    char * line = NULL;
    char * file = NULL;
    size_t len = 0;
    ssize_t nchars;
    int line_index = 0;

    while ((nchars = getline(&line, &len, fp)) != -1)
    {
        if (line_index == 0)
        {
            file = (char *) malloc((strlen(line) + 1) * sizeof(char));
            strcpy(file, line);
        }
        else
        {
            file = (char *)realloc(file, (strlen(line) + strlen(file) + 1) * sizeof(char));
            strcat(file, line);
        }

        line_index++;
    }

    file[strlen(file)-1] = '\0';

    free(line);
    return file;
}


char * get_timestamp()
{
    time_t now = time(NULL);
    char* dt = ctime(&now);
    dt[strlen(dt) - 1] = '\0';
    return dt;
}

char * get_http_timestamp()
{
  char * buf = (char *)malloc(1000 * sizeof(char));
  time_t now = time(0);
  struct tm tm = *gmtime(&now);
  strftime(buf, 1000, "%a, %d %b %Y %H:%M:%S %Z", &tm);
  return buf;
}

char * ms_to_time(int duration)
{
    int ml = (duration%1000)/10;
    int s =(duration/1000)%60;
    int m =(duration/(1000*60))%60;
    int h = (duration/(1000*60*60))%24;

    char h_str[get_number_of_digits(h) + 3];
    char m_str[get_number_of_digits(m) + 3];
    char s_str[get_number_of_digits(s) + 3];
    char ml_str[get_number_of_digits(ml) + 1];

    sprintf(h_str, "%d", h);
    sprintf(m_str, "%d", m);
    sprintf(s_str, "%d", s);
    sprintf(ml_str, "%d", ml);

    char * elapsed_time = (char *)malloc((sizeof(h_str) + sizeof(m_str) + sizeof(s_str) + sizeof(ml_str) + 1) * sizeof(char));

    if (h < 10) { strcpy(elapsed_time, "0"); }
    strcat(elapsed_time, h_str);
    strcat(elapsed_time, ":");

    if (m < 10) { strcat(elapsed_time, "0"); }
    strcat(elapsed_time, m_str);
    strcat(elapsed_time, ":");

    if (s < 10) { strcat(elapsed_time, "0"); }
    strcat(elapsed_time, s_str);
    strcat(elapsed_time, ".");

    strcat(elapsed_time, ml_str);

    //cout<<"'"<<elapsed_time<<"'"<<endl;
    //cout<<h<<"-"<<m<<"-"<<s<<"-"<<ml<<endl;
    return elapsed_time;
}


char * get_stats_str(bool server, unsigned int pages, unsigned int bytes, char * elapsed_time_str)
{
    const char * ans_p1;
    const char * ans_p2;

    if(server)
    {
        ans_p1 = "Server up for ";
        ans_p2 = ", served ";
    }
    else
    {
        ans_p1 = "Crawler up for ";
        ans_p2 = ", downloaded ";
    }

    const char * ans_p3 = " pages, ";
    const char * ans_p4 = " bytes\r\n";

    char ans_p6[get_number_of_digits(pages) + 1];
    sprintf(ans_p6, "%d", pages);

    char ans_p7[get_number_of_digits(bytes) + 1];
    sprintf(ans_p7, "%d", bytes);

    char * stats_str = (char *)malloc((strlen(ans_p1) + strlen(ans_p2)+ strlen(ans_p3)
                        + strlen(ans_p4) + strlen(ans_p6) + strlen(ans_p7)
                        + strlen(elapsed_time_str) + 1) * sizeof(char));

    strcpy(stats_str, ans_p1);
    strcat(stats_str, elapsed_time_str);
    strcat(stats_str, ans_p2);
    strcat(stats_str, ans_p6);
    strcat(stats_str, ans_p3);
    strcat(stats_str, ans_p7);
    strcat(stats_str, ans_p4);

    return stats_str;
}

void bind_on_port(int sock, int port)
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &address, sizeof(address)) <0 )
    {
        perror("Socket bind failed!");
        exit(1);
    }
}



unsigned int get_number_of_digits(int num)
{
    if (num == 0)
    {
        return 1;
    }
    else
    {
        return floor(log10(abs(num))) + 1;   //https://stackoverflow.com/questions/3068397/finding-the-length-of-an-integer-in-c
    }
}


//------------------------------------------------------------------



char ** parse_text_file(FILE * fp, unsigned int & n_lines, unsigned int & n_chars)
{// storing docfile's lines into docs array
    char * line = NULL;
    size_t len = 0;
    ssize_t nchars;
    n_lines = 0;
    n_chars = 0;

    char ** lines = NULL;

    while ((nchars = getline(&line, &len, fp)) != -1)
    {
        if ( line[nchars - 1] == '\n')
        { // removing newline character from string
            line[nchars - 1] = '\0';
            nchars--;
        }

        n_chars += nchars + 1;

        lines = (char **) realloc(lines, (n_lines + 1) * sizeof(char *));
        lines[n_lines] = (char *) malloc((strlen(line) + 1) * sizeof(char));  //plus one for \0
        strcpy(lines[n_lines], line);  //storing docfile's line into doc array
        n_lines++;
    }

    free(line);

    if (n_chars > 0)
    {
        n_chars--;
    }

    return lines;
}


int scan_dir_for_files(char * in_dir)
{//checking that text files at every dir exist
    DIR * FD;
    struct dirent* in_file;

    /* Scanning directory */
    if (NULL == (FD = opendir(in_dir)))
    {
        fprintf(stderr, "Error : Failed to open input directory - %s\n", strerror(errno));
        return 1;
    }

    while ((in_file = readdir(FD)))
    {
        if (!strcmp (in_file->d_name, "."))
            continue;
        if (!strcmp (in_file->d_name, ".."))
            continue;

        FILE * in_dir_file = open_file(in_file->d_name);
        if (in_dir_file == NULL)
        {
            fprintf(stderr, "Error : Failed to open directory file - %s\n", strerror(errno));
            return 1;
        }

        fclose(in_dir_file);
    }

    return 0;
}


char * make_full_file_path(char * path, char * text_file_name)
{
    char * full_file_path = (char *)malloc((strlen(text_file_name) + strlen(path) + 1 + 1) * sizeof(char));
    strcpy(full_file_path, path);
    strcat(full_file_path, "/");
    strcat(full_file_path, text_file_name);
    return full_file_path;
}


int send_msg(const char * msg, unsigned int fd)
{//send a string of uknown legth through fifo
    int buf_size = strlen(msg) + 1;
    if (write(fd, &buf_size, sizeof(int)) < 0)
    {
        perror("problem in writing fifo");
        exit(4);
    }

    if (write(fd, msg, buf_size) < 0)
    {
        perror("problem in writing fifo");
        exit(4);
    }

    return 0;
}


char * read_msg(unsigned int fd)
{//read a string of uknown legth from fifo
    int buf_size;
    if (read(fd, &buf_size, sizeof(int)) < 0)
    {
        perror("problem in reading fifo");
        exit(5);
    }

    char * inc_msg = (char *)malloc(buf_size);
    if (read(fd, inc_msg, buf_size) < 0)
    {
        perror("problem in reading fifo");
        exit(5);
    }

    return inc_msg;
}


int send_int_number(unsigned int fd, int number)
{
    if (write(fd, &number, sizeof(int)) < 0)
    {
        perror("problem in writing fifo");
        exit(4);
    }

    return 0;
}


int read_int_number(unsigned int fd)
{
    int number;
    if (read(fd, &number, sizeof(int)) < 0)
    {
        perror("problem in reading fifo");
        exit(5);
    }

    return number;
}


