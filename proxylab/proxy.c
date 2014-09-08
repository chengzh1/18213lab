/*
 * 15-213 Proxy Lab - proxy.c
 * Name: Cheng Zhang, Andrew ID: chengzh1
 * Name: Zhe Qian, Andrew ID: zheq
 */

#include <stdio.h>
#include <stdlib.h>
#include "csapp.h"
#include "cache.h"

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";
static const char *default_http_version = "HTTP/1.0\r\n";

static cache_list *cache_inst;

void doit(int fd);
int generate_request(rio_t *rp, char *i_request, char *i_host, 
        char *i_uri, int *i_port);
int parse_reqline(char *new_request, char *reqline, 
        char *host, char *uri, int *port);
int parse_uri(char *uri, char *host, int *port, char *uri_nohost);
void get_key_value(char *header_line, char *key, char *value);
void get_host_port(char *value, char *host, int *port);
void *thread(void *vargp);

/* Customized response func */
void client_error(int fd, char *cause, char *errnum, 
        char *shortmsg, char *longmsg);

/* Customized r/w func and error handler wrapper */
int iOpen_clientfd_r(int fd, char *hostname, int port);
ssize_t iRio_readnb(int fd, char *hostname, rio_t *rp, 
        void *usrbuf, size_t n);
int iRio_writen(int fd, void *usrbuf, size_t n);
void iClose(int fd);


int main(int argc, char **argv) {
    int listenfd, port, clientlen;
    int *connfdp;
    struct sockaddr_in clientaddr;
    pthread_t tid;

    /* Cache list initiation */
    cache_inst = (cache_list *)malloc(sizeof(cache_list));
    init_cache_list(cache_inst);

    /* Check command line args number */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);

    /* Ignore SIGPIPE signal */
    Signal(SIGPIPE, SIG_IGN);

    /* Open listening port */
    listenfd = Open_listenfd(port);

    while (1) {
        clientlen = sizeof(clientaddr);
        connfdp = (int *)malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *)&clientaddr, 
                    (socklen_t *)&clientlen);
        Pthread_create(&tid, NULL, thread, connfdp);
    }
    
    return 0;
}

/* 
 * Process the request 
 */
void doit(int fd) {
    rio_t client_rio;
    rio_t server_rio;
    unsigned int total = 0;
    int fit_size = 1;
    char* content_copy = NULL;
    int content_size = 0;

    char *uri = (char *)malloc(MAXLINE * sizeof(char));
    char *request = (char *)malloc(MAXLINE * sizeof(char));
    char *host = (char *)malloc(MAXLINE * sizeof(char));
    int port;
    int server_fd;

    Rio_readinitb(&client_rio, fd);

    /* Check if the request is a GET request */
    int is_get = generate_request(&client_rio, request, host, uri, &port);   
    if(!is_get) {
        free(request);
        free(host);
        free(uri);
        return;
    }

    /* First: read in cache */
    content_copy = read_cache(cache_inst, request, &content_size);
    /* Cache hit: send cached response back to client */
    if (content_size > 0){ 
        if (content_copy == NULL){
            printf("content in cache error\n");
            return;
        }

        iRio_writen(fd, content_copy, content_size);
        free(content_copy);
        free(request);
        free(host);
        free(uri);
        return;
    }  

    /* Cache miss: connect to server to get response */
    server_fd = iOpen_clientfd_r(fd, host, port);
    /* Open connection error */
    if (server_fd < 0) {   
        free(request);
        free(host);
        free(uri);
        return;
    }
        
    Rio_readinitb(&server_rio, server_fd);
    int server_connect = 0;
    /* Send request to server */
    server_connect = iRio_writen(server_fd, request, strlen(request));
    if (server_connect < 0) {
        iClose(server_fd);
        free(request);
        free(host);
        free(uri);
        return;
    }

    /* Forward response from the server to the client through connfd */
    ssize_t nread;
    char buf[MAX_OBJECT_SIZE];
    char content[MAX_OBJECT_SIZE];

    /* Keep reading until the end of response */
    while ((nread = iRio_readnb(fd, host, &server_rio, 
                buf, MAX_OBJECT_SIZE)) != 0) {
        if(nread < 0) {
            free(request);
            free(host);
            free(uri);
            return;
        }

        /* store the response and check if it extends the max object size*/
        if ((total + nread) < MAX_OBJECT_SIZE){
            memcpy(content + total, buf, sizeof(char) * nread);
            total += nread;
        }else{
            printf("web content object is too lage!\n");
            fit_size = 0;
        }
        /* Forward response back to client */
        iRio_writen(fd, buf, nread);
    }

    /* Close proxy-server connection */
    iClose(server_fd);

    /* Cache the response object if it fit the max object size */
    if (fit_size == 1){
        if (strstr(content, "no-cache") != NULL){
            printf("cache control is no cache, do not cache\n");
        }else{
            printf("cache the web content object uri: %s\n", uri);
            modify_cache(cache_inst, request, content, total);
        }
    } 
 
    free(request);
    free(host);
    free(uri);
    return;
}

/* 
 * Generate a new request for server according to the request from clinet
 */
int generate_request(rio_t *rp, char *i_request, char *i_host, 
            char *i_uri, int *i_port) {
    char buf[MAXLINE]; 
    char key[MAXLINE];
    char value[MAXLINE];
    char raw[MAXLINE]; 
    int port = 80;
    int host_in_reqbody = 0; 
    char* request = i_request;
    char* host = i_host;
    char* uri = i_uri;

    *buf = 0;
    *key = 0;
    *value = 0;
    *raw = 0;
    *request = 0;
    *host = 0;

    if (rio_readlineb(rp, buf, MAXLINE) < 0){
        printf("rio_readlineb error\n");
        return 0;
    }

    /* Store the raw request, for debugging */
    strcat(raw, buf);

    /* Parse the request line to get the host, uri and port */
    int is_get = parse_reqline(request, buf, host, uri, &port);

    if(!is_get)
        return 0;

    /* Concat the specified request header */
    strcat(request, user_agent_hdr);
    strcat(request, accept_hdr);
    strcat(request, accept_encoding_hdr);
    strcat(request, connection_hdr);
    strcat(request, proxy_connection_hdr);

    /* Go through the request line by line */
    while (strcmp(buf, "\r\n")) {
        *key = '\0';
        *value = '\0';
        if (rio_readlineb(rp, buf, MAXLINE) < 0){
            printf("rio_readlineb error\n");
            return 0;
        }
        strcat(raw, buf);

        if (!strcmp(buf, "\r\n"))
            break;

        /* Extract one key-value pair from one header line */
        get_key_value(buf, key, value);
        if (*key != '\0' && *value!='\0') {
            /* If the request body has Host header itself, use it */
            if (!strcmp(key, "Host")) {
                get_host_port(value, host, &port);
                host_in_reqbody = 1;
            }
            /* If the key-value pair is not specified, add it */
            if (strcmp(key, "User-Agent") && 
                    strcmp(key, "Accept") && 
                    strcmp(key, "Accept-Encoding") &&
                    strcmp(key, "Connection") &&
                    strcmp(key, "Proxy-Connection")) {

                char hdrline[MAXLINE];
                sprintf(hdrline, "%s: %s\r\n", key, value);
                strcat(request, hdrline);
            }
        }
    }
    /*
     * If request doesn't have a Host header, 
     * combine the host and port from the 
     * first request line as the Host header
     */
    if (!host_in_reqbody) {
        char host_hdr[MAXLINE];
        if (port != 80)
            sprintf(host_hdr, "Host: %s:%d\r\n", host, port);
        else 
            sprintf(host_hdr, "Host: %s\r\n", host);

        strcat(request, host_hdr);
    }

    *i_port = port;

    /* End the request with "\r\n" */
    strcat(request, "\r\n");

    return 1;
}

/* 
 * Process the request line 
 */
int parse_reqline(char *new_request, char *reqline, char *host, 
            char *uri, int *port) {
    char method[MAXLINE], version[MAXLINE];
    char uri_nohost[MAXLINE];
    char new_req[MAXLINE];
    
    /* Get method, uri, and version from the request line */
    sscanf(reqline, "%s %s %s", method, uri, version);

    /* Check if the method is GET */
    if(strcasecmp(method, "GET"))
        return 0;

    /* Parse the uri accordingly */
    parse_uri(uri, host, port, uri_nohost);

    /* Generate a new request line as required */
    sprintf(new_req, "%s %s %s", method, uri_nohost, default_http_version);
    strcat(new_request, new_req);
    return 1;
}

/*
 * Process the uri string
 */
int parse_uri(char *uri, char *host, int *port, char *uri_nohost) {
    char *uri_ptr, *first_slash_ptr;
    char host_str[MAXLINE], port_str[MAXLINE];
    *host = 0;
    *port = 80;

    /* Find the start of "http://" */
    uri_ptr = strstr(uri, "http://");        

    /* If not start with "http://", just return with the uri */
    if (uri_ptr == NULL) {
        strcpy(uri_nohost, uri);
        return 0;
    } else {
        /* Get to the start of the Host string */
        uri_ptr += 7;

        /* Find the end of the Host string */
        first_slash_ptr = strchr(uri_ptr,'/');
        *first_slash_ptr = 0;

        /* Get the Host string */
        strcpy(host_str, uri_ptr);

        *first_slash_ptr = '/';
        strcpy(uri_nohost, first_slash_ptr);

        char *port_ptr;

        /* Get to the start of the Port string */
        port_ptr = strstr(host_str, ":");
        if (port_ptr != NULL) {
                *port_ptr = 0;
                strcpy(port_str, port_ptr + 1);
                *port = atoi(port_str);
        }

        /* Copy the Host string back to the given pointer */
        strcpy(host, host_str);
        return 1;
    }
}

/*
 * Get key value pair from request header line
 */
void get_key_value(char *header_line, char *key, char *value) {
    char *key_tail, *value_tail;

    /* Split the given header line by ":" */
    key_tail = strstr(header_line, ":");
    if (key_tail != NULL) {
        /* Get the key part */
        *key_tail = 0;
        strcpy(key, header_line);
        *key_tail = ':';

        /* Find the "\r\n" and get the value accordingly */
        value_tail = strstr(header_line, "\r");
        *value_tail = 0; 
        strcpy(value, key_tail + 2);
        *value_tail = '\r'; 
    } 
    return;
}

/* 
 * Get Host and Port from the "Host:" request header line
 */
void get_host_port(char *value, char *host, int *port) {
    char *host_tail;
    *port = 80;

    /* Split the given header line by ":" */
    host_tail = strstr(value, ":");
    if (host_tail != NULL) {
        *host_tail = 0;
        strcpy(host, value);
        *port = atoi(host_tail + 1);
        *host_tail = ':';
    }
    return;
}

/* 
 * New thread to process the request 
 */
void *thread(void* vargp) {
    int connfd = *((int *)vargp);
    free(vargp);
    /* 
     * Detach the new thread so that 
     * it can be handled automatically
     * after it finishes
     */
    Pthread_detach(Pthread_self());
    doit(connfd);
    /* Close the connection*/
    iClose(connfd);
    return NULL;
}

/* 
 * Customized r/w func and error handler wrapper 
 */
int iRio_writen(int fd, void *usrbuf, size_t n) {
    if (rio_writen(fd, usrbuf, n) != n) {
        if (errno == EPIPE || errno == ECONNRESET) {
            printf("Server closed %s\n", strerror(errno));
            return -1;
        } 
        else
            return -1;
    }

    return 0;
}

int iOpen_clientfd_r(int fd, char *hostname, int port) {
    int rc;

    if ((rc = open_clientfd_r(hostname, port)) < 0)
        client_error(fd, hostname, "404", "Not found",
            "Proxy couldn't connect to this server");

    return rc;
}

ssize_t iRio_readnb(int fd, char *hostname, rio_t *rp, 
            void *usrbuf, size_t n) {
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0)
        client_error(fd, hostname, "404", "Not found",
            "Proxy couldn't connect to this server");

    return rc;
}

void iClose(int fd){
    if (close(fd) < 0)
        printf("fd close error\n");
}

/* 
 * Build a simple website for cannot connect to server errors 
 */
void client_error(int fd, char *cause, char *errnum, 
            char *shortmsg, char *longmsg) {
    char buf[MAXLINE], body[MAXLINE];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Request Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The proxy</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}