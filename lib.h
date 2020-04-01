#ifndef LIB
#define LIB

#define IDLEN 11
#define TOPICLEN 51
#define INT 0
#define SHORT_REAL 1
#define FLOAT 2
#define STRING 3


typedef struct my_struct3{
  int lport;
  int sent;
  int port[BUFLEN];
  char msg[BUFLEN];
}msg;

typedef struct my_struct2
{
  int SF;
  char topic[TOPICLEN];
  int subscribed;
} topic;

typedef struct my_struct
{
  int port;
  char id_client[IDLEN];
  topic topics[BUFLEN];
  int connected;
} client;

int max(int a, int b)
{
  return a >= b ? a : b;
}


#endif
