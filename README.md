# Web-Server-Crawler


### Table of Contents

[Introduction](#introduction)

[Web-Creator](#web-creator)

[Web-Server](#web-server)

[Web-Crawler](#web-crawler)

[How to compile](#how-to-compile)


<a name="introduction"/>

## Introduction

In this project three different, collaborative applications were implemented:
- ***a shell application*** that creates a set of websites on the disk.
- ***a web server*** that accepts HTTP requests for specific webpages from the websites.
- ***a web crawler***, whose goal is to download all websites from the web server and serve search commands, as well.

These applications are similar (in a simplified form) to those used by popular search engines like Google.

The project encapsulates the whole [Distributed-File-Management-Tool project](https://github.com/JohnPapad/Distributed-File-Management-Tool). 

It was developed for the class of ***"System Programing"*** in the Informatics Department, using **C++** *(without STL)* and **Object Oriented Programming practices**.


<a name="web-creator"/>

## Web-Creator

It is a bash script that generates a set of websites, in a random way, using an input text file.

To run the script use the command:   
```$ ./webcreator.sh <root_directory> <text_file> w p```

**The [webcreator.sh](webcreator.sh) application works as follows:**   

It creates *w* directories, in the *root_directory*, that each represents a website. These directories will be in the form of ```site0, site1, ..., sitew-1```. Then it creates *p* pages in each directory, that represent the webpages of each website. The websites will be in the form of ```pagei_j.html``` where ```i``` is the website and ```j``` is a random, unique for each website, number.  

For example, if ```w = 3 and p = 2``` the following directories and pages could be created:
```
root_dir/site0/  
root_dir/site0/page0_2345.html  
root_dir/site0/page0_2391.html  
root_dir/site1/  
root_dir/site1/page1_2345.html  
root_dir/site1/page1_2401.html  
root_dir/site2/  
root_dir/site2/page2_6530.html  
root_dir/site2/page2_16672.html 
```

<br/>

Each webpage will have the following format:
```
<!DOCTYPE html>  
<html>
    <body>  
        word1  word2  ...  <a  href=”link1”>link1_text</a>    
        wordn  wordn+1  ...  <a  href=”link2”>link2_text</a>  wordm  wordm+1  ...  
    </body>  
</html>  
```

### Command line arguments

- ***root_directory***: is a directory that must already exist and in which the websites will be generated. If the *root_directory* is not empty the script will purge it.
- ***text_file​***: is a text file, from which random words will be selected to create the
webpages. One such file is [J. Verne's "Twenty Thousand Leagues under the Sea"](http://www.gutenberg.org/cache/epub/164/pg164.txt), which is included. It must contain at least 10000 lines.
- ***w***: the number of the websites that will be created.
- ***p***: the number of the webpages, per website, that will be created.

> Constraints: w >= 2, p >= 3 


<a name="web-server"/>

## Web-Server

To run the application use the command:   
```$ ./myhttpd -­p <serving_port> -­c <command_port> -­t <num_of_threads> -­d <root_dir> ```

In the *service port* the server simply accepts *HTTP/ 1.1* requests. In this project only a simple *GET* request is implemented. The server is listening to the specific port for requests of the following format:
```
GET  /site0/page0_1244.html  HTTP/1.1  
User-­Agent:  Mozilla/4.0  (compatible;  MSIE5.01;  Windows  NT)  
Host:  www.tutorialspoint.com  
Accept-­Language:  en-­us  
Accept-­Encoding:  gzip,  deflate  
Connection:  Keep-­Alive  
[blank  line  here]  
```

When it receives such a request, the server assigns it to one of the threads from the thread pool. The thread's job is to return the ```root_dir/site0/page0_1244.html``` file back. 

**The operation of the server with the threads is as follows:**   
The server accepts a connection to the socket and places the corresponding file descriptor in a buffer from which the threads read. Each thread is responsible for a file descriptor, from which it will
read the *GET* request and return the answer, i.e. the webpage's content *(or a different answer, in case the file does not exist, or the thread does not have the appropriate permissions, as depicted below)*.

- If the requested file exists, and the server has the permission to read it, then a response of the following format is returned:
    ```
    HTTP/1.1  200  OK  
    Date:  Mon,  27  May  2018  12:28:53  GMT  
    Server:  myhttpd/1.0.0  (Ubuntu64)  
    Content-­Length:  8873  
    Content-­Type:  text/html  
    Connection:  Closed  
    [blank  line]  
    [content  of  the  requested  file  here...  e.g.  <html>hello  one  two  ...</html>]
    ```
    > The length is in bytes, expressing only the size of the content, i.e. without including he
    header.

- If the requested file does not exist, then the server return a response of the following format:
    ```
    HTTP/1.1  404  Not  Found  
    Date:  Mon,  27  May  2018  12:28:53  GMT  
    Server:  myhttpd/1.0.0  (Ubuntu64)  
    Content-­Length:  124  
    Content-­Type:  text/html  
    Connection:  Closed  
    [blank  line]  
    <html>Sorry  dude,  couldn’t  find  this  file.</html>  
    ```

- If the server does not the have the appropriate permissions, then a response of the following format is returned:
    ```
    HTTP/1.1  403  Forbidden  
    Date:  Mon,  27  May  2018  12:28:53  GMT  
    Server:  myhttpd/1.0.0  (Ubuntu64)  
    Content-­Length:  124  
    Content-­Type:  text/html  
    Connection:  Closed  
    [blank  line]  
    <html>Trying  to  access  this  file  but  don’t  think  I  can  make  it.</html>
    ```

### Command line arguments

- ***-p*** *(serving_port)*: the port that the server is listening in order to return webpages.
- ***-c*** *(command_port)*: the port that the server is listening in order to receive commands.
- ***-t*** *( num_of_threads​)*: the number of threads that the server will create in order to
manage incoming requests. These threads are all created together, at the beginning, in a thread pool so that the server can reuse them. In the event that a thread is terminated, the server creates a new one.
- ***-d*** *(root_dir)*: the directory that contains all the websites created by the [webcreator.sh](webcreator.sh).

### Application commands

Furthermore, at the *command port*, the server listens and accepts the following simple commands *(1 word each)*, which are executed directly by the server, without having to be assigned to a thread:

- ***STATS***: the server responds with specific statistics: the duration it runs, how many pages it has returned and the total number of bytes, as well. E.g.:    
```Server  up  for  02:03.45,  served  124  pages,  345539  bytes```
- ***SHUTDOWN​***: the server stops serving additional requests, releases any memory *(shared or not)* has allocated and stops its execution.


<a name="web-crawler"/>

## Web-Crawler

The crawler's job is to download all the websites' webpages from the webserver, analyzing them and finding links within the pages that will follow retrospectively.

To run the application use the command:   
```$ ./mycrawler -­h <host_or_IP> -­p <port> -­c <command_port> -­t <num_of_threads> -­d <save_dir> <starting_URL>``` 

**The crawler works as follows:**   
In the beginning, it creates a thread pool with the corresponding threads. The threads are being reused.
It also creates a queue in order to store the links that have been found so far and appends the *starting_URL* to this queue.   

One of the threads takes the URL from the queue and requests it from the server. The URL is in the format of (for example) ```http://linux01.di.uoa.gr:8080/site1/page0_1234.html```.   

After downloading the file, it saves it to the corresponding directory/file in the *save_dir*.   

It analyzes the file, that has just been downloaded, and finds additional links that appends to the queue.   

It repeats this process with all the threads in parallel, until there are no other links
in the queue.


### Command line arguments
 
- ***-h*** *(host_or_IP)*: the name of the machine or the IP that the server is running.
- ***-p*** *(port)*: the port on which the server is listening.
- ***-c*** *(command_port)*: the port on which the crawler is listening in order to receive commands.
- ***-t*** *(num_of_threads)*: the number of threads that the crawler creates and runs. In the event that a thread terminates, the crawler creates a new one.
- ***-d*** *(save_dir)*: the directory in which the crawler will save the downloaded pages.
- ***starting_URL***: the URL from which the crawler starts its operation.

### Application commands

Furthermore, at the *command port* the crawler listens and accepts the following simple commands *(1 word each)*, which are
executed directly by the crawler, without having to be assigned to a thread:

- ***STATS***: the crawler responds with specific statistics: the duration it runs, how many pages it has downloaded and the total number of bytes, as well. E.g.:    
```Crawler  up  for  01:06.45,  downloaded  124  pages,  345539  bytes```
- ***SEARCH*** e.g. ```SEARCH  word1  word2  word3  ...  word10```: If the crawler still has pages in the queue, then it returns a message indicating that the crawling is still in progress.   
If the crawler has finished downloading all the webpages, then this command will utilize the [Distributed-File-Management-Tool project's](https://github.com/JohnPapad/Distributed-File-Management-Tool) *search* command and will return the same results, but over the socket. 
- ***SHUTDOWN​***: the crawler stops downloading webpages, releases any memory *(shared or not)* has allocated and stops its execution.


<a name="how-to-compile"/>

## How to compile

A makefile is provided. You can use the following commands:

- ```$ make```: for easy compilation *(both the web server and crawler will be compiled)*.
- ```$ make myhttpd```: to only compile the *web-server* application.
- ```$ make mycrawler```: to only compile the *web-crawler* application.
- ```$ make clean1```: to delete all **.o* auto generated files from the *web-server* application.
- ```$ make clean2```: to delete all **.o* auto generated files from the *web-crawler* application.
- ```$ make cleanall``` to delete all **.o* auto generated files from both applications.
- ```$ make cleanfifos```: to delete all named pipes.

> For execution details and appropriate command line arguments see each application's chapter. 