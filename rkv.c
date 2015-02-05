#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "kv.h"
#include "kv_proto.h"

// Map response code to text for GET and DEL
const char* ResponseText[] = {
  "succeeded",
  "failed",
  "key is null",
  "key is too large",
  "key is not found",
  "key value store is empty"
};

// Map response code to text for PUT
const char* PutResponseText[] = {
  "succeeded",
  "failed",
  "key is null",
  "key is too large",
  "value is null",
  "value is too large",
  "key value pair is null",
  "key is not unique; value is reset",
  "key value store is out of capacity"
};

void put(CLIENT *clnt, char *key, char *value) {
  KeyValue kv;
  int *result;

  // Allocate memory for key 
  kv.key = malloc((strlen(key) + 1) * sizeof(char));

  if (kv.key == NULL) {
    fprintf(stderr, "PUT: client error: memory allocation failure\n");
    exit(1); // Exit on memory allocation failure
  }

  // Copy key to allocated memory
  strcpy(kv.key, key);

  // Allocate memory for value
  kv.value = malloc((strlen(value) + 1) * sizeof(char));

  if (kv.value == NULL) {
    fprintf(stderr, "PUT: client error: memory allocation failure\n");
    exit(1); // Exit on memory allocation failure
  }

  // Copy value to allocated memory
  strcpy(kv.value, value);

  // Perform PUT
  result = put_1(&kv, clnt);

  if (result == (int *)NULL) {
    clnt_perror(clnt, "PUT: rpc error");
    exit(1);
  }

  fprintf(stdout, "PUT: server response: %s (%d)\n", PutResponseText[*result], *result);
  free(kv.key); // Free memory for key
  free(kv.value); // Free memory for value
}

void get(CLIENT *clnt, char *key) {
  char *kv_key;
  GetReply *result;

  kv_key = malloc((strlen(key) + 1) * sizeof(char));

  if (kv_key == NULL) {
    fprintf(stderr, "GET: client error: memory allocation failure\n");
    exit(1);
  }

  strcpy(kv_key, key);

  // Perform GET
  result = get_1(&kv_key, clnt);

  if (result == (GetReply *)NULL) {
    clnt_perror(clnt, "GET: rpc error");
    exit(1);
  }

  fprintf(stdout, "GET: server response: %s (%d)", ResponseText[result->code],  result->code);

  if (result->code == GET_OK) {
    fprintf(stdout, "; value=\"%s\"\n", result->value);
  } else {
    fprintf(stdout, "\n");
  }

  free(kv_key);
}

void del(CLIENT *clnt, char *key) {
  char *kv_key;
  int *result;

  kv_key = malloc((strlen(key) + 1) * sizeof(char));

  if (kv_key == NULL) {
    fprintf(stderr, "DEL: client error: memory allocation failure\n");
    exit(1);
  }

  strcpy(kv_key, key);

  result = del_1(&kv_key, clnt);

  if (result == (int *)NULL) {
    clnt_perror(clnt, "DEL: rpc error");
    exit(1);
  }

  fprintf(stdout, "DEL: server response: %s (%d)\n", ResponseText[*result],  *result);
  free(kv_key);
}

int sendToServer(int argc, char **argv) {
  CLIENT *clnt;
  int *result;
  char *server;
  char *cmd;

  if ((argc < 4) || (argc > 5)) {
    fprintf(stderr, "usage: %s host (PUT|GET|DEL) key [value]\n", argv[0]);
    exit(1);
  }

  cmd = argv[2];

  //
  // Input validation.
  //
  
  if (!strcmp(cmd, "PUT")) {
    if (argc != 5) {
      fprintf(stderr, "usage: %s host PUT key value\n", argv[0]);
      exit(1);
    }
  } else if (!strcmp(cmd, "GET")) {
    if (argc != 4) {
      fprintf(stderr, "usage: %s host GET key\n", argv[0]);
      exit(1);
    }
  } else if (!strcmp(cmd, "DEL")) {
    if (argc != 4) {
      fprintf(stderr, "usage: %s host DEL key\n", argv[0]);
      exit(1);
    }
  } else {
    fprintf(stderr, "bad command\n");
    fprintf(stderr, "usage: %s host (PUT|GET|DEL) key [value]\n", argv[0]);
    exit(1);
  }

  //
  // Establish RPC connection.
  //
    
  server = argv[1];

  clnt = clnt_create(server, KVPROGRAM, KVVERSION, "tcp");

  if (clnt == (CLIENT *)NULL) {
    fprintf(stdout, "client create failed\n");
    clnt_pcreateerror(server);
    exit(1);
  }

  //
  // Handle command.
  //

  if (!strcmp(cmd, "PUT")) {
    put(clnt, argv[3], argv[4]);
  } else if (!strcmp(cmd, "GET")) {
    get(clnt, argv[3]);
  } else if (!strcmp(cmd, "DEL")) {
    del(clnt, argv[3]);
  }

  //
  // Disconnect.
  //

  clnt_destroy(clnt);
  return 0;
}
/*
int main(){
    char* arg1[5] = {"","127.0.0.1","PUT","carrot","[orange]",};
    char* arg2[4] = {"","127.0.0.1","GET","carrot"};
    char* arg3[4] = {"","127.0.0.1","DEL","carrot"};
    
    char* arg4[5] = {"","127.0.0.1","PUT","apple","[red]",};
    char* arg5[4] = {"","127.0.0.1","GET","apple"};
    char* arg6[4] = {"","127.0.0.1","DEL","apple"};
    
    char* arg7[5] = {"","127.0.0.1","PUT","banana","[yellow]",};
    char* arg8[4] = {"","127.0.0.1","GET","banana"};
    char* arg9[4] = {"","127.0.0.1","DEL","banana"};
    
    char* arg10[5] = {"","127.0.0.1","PUT","grapes","[purple]",};
    char* arg11[4] = {"","127.0.0.1","GET","grapes"};
    char* arg12[4] = {"","127.0.0.1","DEL","grapes"};
    
    char* arg13[5] = {"","127.0.0.1","PUT","kiwi","[green]",};
    char* arg14[4] = {"","127.0.0.1","GET","kiwi"};
    char* arg15[4] = {"","127.0.0.1","DEL","kiwi"};

    sendToServer(5, arg1);
    sendToServer(4, arg2);
    sendToServer(4, arg3);
    
    sendToServer(5, arg4);
    sendToServer(4, arg5);
    sendToServer(4, arg6);
    
    sendToServer(5, arg7);
    sendToServer(4, arg8);
    sendToServer(4, arg9);
    
    sendToServer(5, arg10);
    sendToServer(4, arg11);
    sendToServer(4, arg12);
    
    sendToServer(5, arg13);
    sendToServer(4, arg14);
    sendToServer(4, arg15);
}*/
