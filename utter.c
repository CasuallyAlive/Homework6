/*
 * utter.c - [Starting code for] a web-based Ute social network.
 *
 * Based on:
 *  tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *      GET method to serve static and dynamic content.
 *   Tiny Web server
 *   Dave O'Hallaron
 *   Carnegie Mellon University
 */
#include "csapp.h"
#include "dictionary.h"
#include "more_string.h"

static void doit(int fd);
static dictionary_t *read_requesthdrs(rio_t *rp);
static void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);
static void clienterror(int fd, char *cause, char *errnum,
                        char *shortmsg, char *longmsg);
static void print_stringdictionary(dictionary_t *d);

static void serve_request(int fd, char *);

static void do_listen(int, dictionary_t*);
static void do_utter(int, dictionary_t*);
static void do_shh(int, dictionary_t*);
static void do_sync(int, dictionary_t*);
static void makeUserID(char* user, char* id);

struct dictionary_t *users_posts_dict;
char *HOST_NAME, *PORT;
int id_num = 0;
const size_t MAX_ID = 150;

#ifdef SILENCE_PRINTF
#define printf(...)
#endif


int main(int argc, char **argv) {

  users_posts_dict = make_dictionary(COMPARE_CASE_SENS, NULL);

  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  
  HOST_NAME = hostname;
  PORT = port;

  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  /* Don't kill the server if there's an error, because
     we want to survive errors due to a client. But we
     do want to report errors. */
  exit_on_error(0);

  /* Also, don't stop on broken connections: */
  Signal(SIGPIPE, SIG_IGN);

  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (connfd >= 0) {
      Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE,
          port, MAXLINE, 0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);

      doit(connfd);
      Close(connfd);
      printf("connection closed\n");
    }
  }
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd){

  char buf[MAXLINE], *method, *uri, *version;
  rio_t rio;
  dictionary_t *headers, *query;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
    return;
  printf("%s", buf);

  if (!parse_request_line(buf, &method, &uri, &version)) {
    clienterror(fd, method, "400", "Bad Request",
                "Utter did not recognize the request");
  } else {
    if (strcasecmp(version, "HTTP/1.0")
        && strcasecmp(version, "HTTP/1.1")) {
      clienterror(fd, version, "501", "Not Implemented",
          "Utter does not implement that version");
    } else if (strcasecmp(method, "GET")
        && strcasecmp(method, "POST")){
      clienterror(fd, method, "501", "Not Implemented",
          "Utter does not implement that method");
    } else {
      headers = read_requesthdrs(&rio);

      /* Parse all query arguments into a dictionary */
      query = make_dictionary(COMPARE_CASE_SENS, free);
      parse_uriquery(uri, query);
      if (!strcasecmp(method, "POST")){
        read_postquery(&rio, headers, query);
      }
      /*For debugging, print the dictionary */
      printf("query params\n");
      print_stringdictionary(query);
      printf("URI: %s\n", uri);


      /* You'll want to handle different queries here,
         but the intial implementation always returns
         "hello world": */
      if(starts_with("/listen", uri)){
        do_listen(fd, query);
      }
      else if(starts_with("/utter",uri)){
        do_utter(fd, query);
      }
      else if(starts_with("/shh", uri)){
        do_shh(fd, query);
      }
      else if(starts_with("/sync", uri)){
        do_sync(fd, query);
      }
      // serve_request(fd, query);

      /* Clean up */
      free_dictionary(query);
      free_dictionary(headers);
    }

    /* Clean up status line */
    free(method);
    free(uri);
    free(version);

  }

}
/**
 * @brief GET /listen?user=‹user› — Returns the posts made by <user>.  
 * For each post, the response body should contain the utter's ID, a space character, the text of the utter, and then a newline character.  
 * The order of the lines in the response doesn't matter.
 * 
 * @param fd 
 * @param query 
 */
static void do_listen(int fd, dictionary_t* query){

}
/**
 * @brief POST /utter?user=‹user›&utterance=‹the message› — This makes a new utter post for user.  
 * Your server needs to create a unique ID for the post and store it in memory.  
 * The ID needs to be unique across all servers, so I recommend using the server's hostname and port as part of the ID.  
 * The response body should contain the ID.
 * 
 * @param fd 
 * @param query 
 */
static void do_utter(int fd, dictionary_t* query){
  char *user = dictionary_get(query, "user");
  char *message = dictionary_get(query, "utterance");

  if(dictionary_count(query) < 1 && (user == NULL || message == NULL)){
    //report error
      clienterror(fd, "POST","400", "Bad Request",
              "Utter did not recognize the request");
  }

  char id[MAX_ID];
  makeUserID(user, id);
  dictionary_t *user_utter_posts;

  if(dictionary_get(users_posts_dict, user) == NULL){
    user_utter_posts = make_dictionary(COMPARE_CASE_SENS, NULL);

    dictionary_set(user_utter_posts, id, message); //Set of messages
    dictionary_set(users_posts_dict, user, user_utter_posts);
  }else{
    user_utter_posts = dictionary_get(users_posts_dict, user);
    dictionary_set(user_utter_posts, id, message);
  }
  serve_request(fd,id);
}
/**
 * @brief POST /shh?user=‹user›&id=‹messageID› — Remove the post with the given ID from the user's posts. 
 * If no post with that ID exists you can return either success or error (the testing scripts don't intentionally remove posts that don't exist)
 * 
 * @param fd 
 * @param query 
 */
static void do_shh(int fd, dictionary_t* query){

}
/**
 * @brief POST /sync?user=‹user›&hostname=‹hostname›&port=‹port› — Contacts an utter server running on ‹host› at ‹port› to get all of the posts of ‹user›, 
 * and adds those posts to the user's posts on this server. 
 * The given ‹host› and ‹port› should refer to some utter server (the test scripts run another server on localhost using a different port).  
 * 
 * @param fd 
 * @param query 
 */
static void do_sync(int fd, dictionary_t* query){

}

static void makeUserID(char* user, char* id){
  char num_str[10];
  sprintf(num_str, "%d", id_num++);

  strcat(id, HOST_NAME);
  strcat(id, "_");
  strcat(id, PORT);
  strcat(id, "_");
  strcat(id, user);
  strcat(id, "_");
  strcat(id, num_str);
}
/*
 * read_requesthdrs - read HTTP request headers
 */
dictionary_t *read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];
  dictionary_t *d = make_dictionary(COMPARE_CASE_INSENS, free);

  Rio_readlineb(rp, buf, MAXLINE);
  //printf("%s", buf);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    //printf("%s", buf);
    parse_header_line(buf, d);
  }

  return d;
}
// What does this do.
void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *dest) {
  char *len_str, *type, *buffer;
  int len;

  len_str = dictionary_get(headers, "Content-Length");
  len = (len_str ? atoi(len_str) : 0);

  type = dictionary_get(headers, "Content-Type");

  buffer = malloc(len+1);
  Rio_readnb(rp, buffer, len);
  buffer[len] = 0;

  if (!strcasecmp(type, "application/x-www-form-urlencoded")) {
    parse_query(buffer, dest);
  }

  free(buffer);
}
// What does this do
static char *ok_header(size_t len, const char *content_type) {
  char *len_str, *header;

  header = append_strings("HTTP/1.0 200 OK\r\n",
                          "Server: Utter Web Server\r\n",
                          "Connection: close\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n",
                          "Content-type: ", content_type, "\r\n\r\n",
                          NULL);
  free(len_str);

  return header;
}

/*
 * serve_request - example request handler
 */
static void serve_request(int fd, char *body) {
  size_t len;
  char *header;

  // body = strdup("hello world!\n");

  len = strlen(body);

  /* Send response headers to client */
  header = ok_header(len, "text/plain; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);

  free(header);

  /* Send response body to client */
  Rio_writen(fd, body, len);

  // free(body);
}

/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum,
		 char *shortmsg, char *longmsg) {
  size_t len;
  char *header, *body, *len_str;

  body = append_strings("<html><title>Utter Error</title>",
                        "<body bgcolor=""ffffff"">\r\n",
                        errnum, " ", shortmsg,
                        "<p>", longmsg, ": ", cause,
                        "<hr><em>Utter Server</em>\r\n",
                        NULL);
  len = strlen(body);

  /* Print the HTTP response */
  header = append_strings("HTTP/1.0 ", errnum, " ", shortmsg, "\r\n",
                          "Content-type: text/html; charset=utf-8\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n\r\n",
                          NULL);
  free(len_str);

  Rio_writen(fd, header, strlen(header));
  Rio_writen(fd, body, len);

  free(header);
  free(body);
}

static void print_stringdictionary(dictionary_t *d) {
  int i, count;

  count = dictionary_count(d);
  for (i = 0; i < count; i++) {
    printf("%s=%s\n",
           dictionary_key(d, i),
           (const char *)dictionary_value(d, i));
  }
  printf("\n");
}
