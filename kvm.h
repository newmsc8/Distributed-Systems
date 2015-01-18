#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#define FILE_ERR -1
#define DUPLICATE_KEY_ERR -2
#define NO_SUCH_KEY_ERR -3
#define NULL_KEY_ERR -4
#define NULL_VALUE_ERR -5
#define UNKNOWN_ERR -100
#define KEY_VALUE_ERR -6
#define PACKET_SIZE 15000
#define PUT 1
#define GET 2
#define DELETE 3
#define MALFORMED_PACKET -50

int get(char* key, char* value);
int put(char*key, char* value);
int delete_key(char* kry);
void init_key_values();
int parse_package(char* package, char* results[]);