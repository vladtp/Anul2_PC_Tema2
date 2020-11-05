#include <netinet/in.h>
#include <string>

#define DIE(assertion, call_description)\
    if (assertion) {					\
        fprintf(stderr, "(%s, %d): ",	\
                __FILE__, __LINE__);	\
        perror(call_description);		\
        exit(EXIT_FAILURE);				\
    }	

#define MAX_CLIENTS 1000
#define BUFF_LEN 2000
#define ID_SIZE 10

struct cli_msg {
    unsigned char op;
    char topic[50];
    unsigned char sf;
};

struct srv_msg
{
    char topic[51];
    uint8_t type;
    char payload[1501];
    in_addr ip_addr;
    in_port_t port_addr;
};

struct subscriber
{
    std::string id;
    unsigned char sf;
};


#define ID_OP 0
#define SUB_OP 1
#define UNSUB_OP 2
#define INT_TYPE 0
#define SHORT_TYPE 1
#define FLOAT_TYPE 2
#define STRING_TYPE 3

