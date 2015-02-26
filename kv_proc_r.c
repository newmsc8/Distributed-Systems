#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "kv.h"
#include "kv_proto.h"

#define MAX_REPLICA 4

const char* ResponseText[] = {
  "succeeded",
  "failed",
  "key is null",
  "key is too large",
  "key is not found",
  "key value store is empty"
};

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

char KvStore[KVSTORE_CAPACITY][2][MAX_LENGTH];
int NextSlot;

char ReplicaName[MAX_REPLICA][MAX_LENGTH];
int ReplicaCount;

CLIENT *Replica[MAX_REPLICA];

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; //initialize a lock
struct thread_data{
  char* key;
  CLIENT* clnt;
  KeyValue key_value;
};
pthread_t acceptor;
pthread_t learner;
pthread_t proposer;

int read_replica_names_from_file() {
  FILE *f;
  char name[MAX_LENGTH];

  f = fopen("replica", "rt");

  if (f == NULL) {
    fprintf(stderr, "Can't open file: \"replica\"\n");
    return 0; // Failure.
  }

  ReplicaCount = 0;

  while(fscanf(f, "%s", name) != -1) {
    strcpy(ReplicaName[ReplicaCount++], name);

    if (ReplicaCount == MAX_REPLICA) {
      break;
    }
  }

  fclose(f);
  return 1; // Success.
}

int open_connection_to_replicas() {
  int i;

  for (i = 0; i < ReplicaCount; i++) {
    Replica[i] = clnt_create(ReplicaName[i], KVPROGRAM, KVVERSION, "tcp");

    if (Replica[i] == (CLIENT *)NULL) {
      fprintf(stderr, "Connection to replica: \"%s\" failed.\n", ReplicaName[i]);
      clnt_pcreateerror(ReplicaName[i]);

      //
      // Disconnect already opened connections.
      //

      for (i--; i >= 0; i--) {
	clnt_destroy(Replica[i]);
	Replica[i] = NULL;
      }

      return 0; // Failure.
    }
  }

  return 1; // Success.
}

void close_connection_to_replicas() {
  int i;

  for (i = 0; i < ReplicaCount; i++) {
    if (Replica[i] != NULL) {
      clnt_destroy(Replica[i]);
      Replica[i] = NULL;
    }
  }
}

int * thread_put(void *threadargs) {
  static int result;
  struct thread_data *put_data;
  KeyValue key_value;
  CLIENT* clnt;
  
  put_data=(struct thread_data*)threadargs;
  key_value=put_data->key_value;
  clnt=put_data->clnt;

  result=rput_1(&key_value,clnt);

  return(&result);
}

int * thread_get(void *threadargs) {
  static int result;
  struct thread_data *get_data;
  char* key;
  CLIENT* clnt;
  
  get_data=(struct thread_data*)threadargs;
  key=get_data->key;
  clnt=get_data->clnt;

  result=rget_1(&key,clnt);

  return(&result);
}

int * thread_del(void *threadargs) {
  static int result;
  struct thread_data *del_data;
  char* key;
  CLIENT* clnt;
  
  del_data=(struct thread_data*)threadargs;
  key=del_data->key;
  clnt=del_data->clnt;

  result=rdel_1(&key,clnt);

  return(&result);
}

int rput(CLIENT *clnt, char *key, char *value) {
  KeyValue kv;
  int *result;

  kv.key = malloc((strlen(key) + 1) * sizeof(char));

  if (kv.key == NULL) {
    fprintf(stderr, "PUT: server error: memory allocation failure\n");
    return PUT_FAILED;
  }

  strcpy(kv.key, key);

  kv.value = malloc((strlen(value) + 1) * sizeof(char));

  if (kv.value == NULL) {
    fprintf(stderr, "PUT: server error: memory allocation failure\n");
    
    free(kv.key);
    return PUT_FAILED;
  }

  strcpy(kv.value, value);

  //about to perform delete function
  //don't preceed until you get a lock
  pthread_mutex_lock(&lock);

  struct thread_data pth_data;
  pth_data.key_value = kv;
  pth_data.clnt = clnt;
  pthread_t pth;
  pthread_create(&pth,NULL,thread_put,(void*)&pth_data);
  pthread_join(pth,(void*)&result);

  //completed delete function
  //release lock
  pthread_mutex_unlock(&lock);

  //result = rput_1(&kv, clnt);


  if (result == (int *)NULL) {
    clnt_perror(clnt, "PUT: rpc error");
    
    free(kv.key);
    free(kv.value);
    return PUT_FAILED;
  }

  fprintf(stdout, "PUT: replication response: %s (%d)\n", PutResponseText[*result], *result);

  free(kv.key);
  free(kv.value);

  return *result;
}

int rget(CLIENT *clnt, char *key, char *refValue) {
  char *kv_key;
  GetReply *result;

  kv_key = malloc((strlen(key) + 1) * sizeof(char));

  if (kv_key == NULL) {
    fprintf(stderr, "GET: server error: memory allocation failure\n");
    return GET_FAILED;
  }

  strcpy(kv_key, key);

  struct thread_data pth_data;
  pth_data.key = kv_key;
  pth_data.clnt = clnt;
  pthread_t pth;
  pthread_create(&pth,NULL,thread_get,(void*)&pth_data);
  pthread_join(pth,(void*)&result);

  //result = rget_1(&kv_key, clnt);

  if (result == (GetReply *)NULL) {
    clnt_perror(clnt, "GET: rpc error");

    free(kv_key);
    return GET_FAILED;
  }

  fprintf(stdout, "GET: replication response: %s (%d)", ResponseText[result->code],  result->code);

  if (result->code == GET_OK) {
    fprintf(stdout, "; value=\"%s\"\n", result->value);

    if (strcmp(result->value, refValue)) {
      free(kv_key);
      return GET_FAILED;
    }
  } else {
    fprintf(stdout, "\n");

    free(kv_key);
    return GET_FAILED;
  }

  free(kv_key);
  return GET_OK;
}

int rdel(CLIENT *clnt, char *key) {
  char *kv_key;
  int *result;

  kv_key = malloc((strlen(key) + 1) * sizeof(char));

  if (kv_key == NULL) {
    fprintf(stderr, "DEL: server error: memory allocation failure\n");
    return DEL_FAILED;
  }

  strcpy(kv_key, key);
  
  //about to perform delete function
  //don't preceed until you get a lock
  pthread_mutex_lock(&lock);

  struct thread_data pth_data;
  pth_data.key = kv_key;
  pth_data.clnt = clnt;
  pthread_t pth;
  pthread_create(&pth,NULL,thread_del,(void*)&pth_data);
  pthread_join(pth,(void*)&result);

  pthread_mutex_unlock(&lock);

  if (result == (int *)NULL) {
    clnt_perror(clnt, "DEL: rpc error");

    free(kv_key);
    return DEL_FAILED;
  }

  fprintf(stdout, "DEL: replication response: %s (%d)\n", ResponseText[*result],  *result);

  free(kv_key);
  return DEL_OK;
}

void replicate_begin() {
  read_replica_names_from_file();
  open_connection_to_replicas();
}

void replicate_end() {
  close_connection_to_replicas();
}

//int* del_thread(void* thread_args) {
 // struct thread_data *my_data;
  //my_data = (struct thread_data*)thread_args;

  //char * key;
  //struct svc_req* clnt;

  //key = (char*)my_data->key;
  //clnt = (struct svc_req*)my_data->clnt;
  
  //return rdel_1_svc(&key,clnt);
//}

int * put_1_svc(KeyValue *kv, struct svc_req *req) {
  static int result; /* must be static */
  int i;

  //
  // First check if the PUT operation is viable on this replica.
  //

  result = *vote_put_1_svc(kv, req); // local call.

  if (result == PUT_VOTE_NO) {

    //
    // The PUT operation is not viable locally. No need to talk to other replicas.
    // Client will see PUT_FAILED
    // Because PUT_VOTE_NO == PUT_FAILED
    //
    
    return (&result);
  }

  replicate_begin();

  //
  // Ask for vote from other replicas.
  // If any of the replica votes NO then the operation fails.
  //

  for (i = 0; i < ReplicaCount; i++) {
    if ((result = *vote_put_1(kv, Replica[i]) /* rpc */) == PUT_VOTE_NO) {
      replicate_end();
      return (&result); 
    }
  }

  //
  // Every replica has voted YES.
  //

  //
  // Do local PUT.
  //

  result = *rput_1_svc(kv, req); // local call.

  if (result != PUT_OK) {
    replicate_end();
    return (&result);
  }

  //
  // Do remote PUT (replicate).
  //

  for (i = 0; i < ReplicaCount; i++) {
    if ((result = rput(Replica[i], kv->key, kv->value)) != PUT_OK) {
      replicate_end();
      return (&result); 
    }
  }

  replicate_end();

  return (&result);
}

GetReply * get_1_svc(char **key, struct svc_req *req) {
  static GetReply result; /* must be static */
  int status;
  int i;
  
  result = *rget_1_svc(key, req);

  if (result.code != GET_OK) {
    return (&result);
  }

  replicate_begin();

  for (i = 0; i < ReplicaCount; i++) {
    if ((status = rget(Replica[i], *key, result.value)) != GET_OK) {
      strcpy(result.value, "");
      result.code = status;

      replicate_end();
      return (&result);
    }
  }

  replicate_end();

  return (&result);
}

int * del_1_svc(char **key, struct svc_req *req) {
  static int result; /* must be static */
  int i;

  //
  // First check if the DEL operation is viable on this replica.
  //

  result = *vote_del_1_svc(key, req); // local call.

  if (result == DEL_VOTE_NO) {

    //
    // The DEL operation is not viable locally. No need to talk to other replicas.
    // Client will see DEL_FAILED
    // Because DEL_VOTE_NO == DEL_FAILED
    //
    
    return (&result);
  }

  replicate_begin();

  //
  // Ask for vote from other replicas.
  // If any of the replica votes NO then the operation fails.
  //

  for (i = 0; i < ReplicaCount; i++) {
    if ((result = *vote_del_1(key, Replica[i]) /* rpc */) == DEL_VOTE_NO) {
      replicate_end();
      return (&result); 
    }
  }

  //
  // Every replica has voted YES.
  //

  //
  // Do local DEL.
  //

  result = *rdel_1_svc(key, req); // local call.

  if (result != DEL_OK) {
    replicate_end();
    return (&result);
  }

  //
  // Do remote DEL (replicate).
  //

  for (i = 0; i < ReplicaCount; i++) {
    if ((result = rdel(Replica[i], *key)) != DEL_OK) {
      replicate_end();
      return (&result); 
    }
  }

  replicate_end();

  return (&result);
}

int * rput_1_svc(KeyValue *kv, struct svc_req *req) {
  static int result; /* must be static */
  int i;

  if (kv == NULL) {
    fprintf(stderr, "PUT: null key value pair\n");
    result = PUT_KVPAIR_IS_NULL;
    return (&result);
  }

  if (kv->key == NULL) {
    fprintf(stderr, "PUT: key is null\n");
    result = PUT_KEY_IS_NULL;
    return (&result);
  }

  if (strlen(kv->key) + 1 >= MAX_LENGTH) {
    fprintf(stderr, "PUT: key is too large\n");
    result = PUT_KEY_IS_TOO_LARGE;
    return (&result);
  }

  if (kv->value == NULL) {
    fprintf(stderr, "PUT: value is null\n");
    result = PUT_VALUE_IS_NULL;
    return (&result);
  }

  if (strlen(kv->value) + 1 >= MAX_LENGTH) {
    fprintf(stderr, "PUT: value is too large\n");
    result = PUT_VALUE_IS_TOO_LARGE;
    return (&result);
  }

  if (NextSlot == KVSTORE_CAPACITY) {
    fprintf(stderr, "PUT: key value store is out of capacity\n");
    result = PUT_KVSTORE_OUT_OF_CAPACITY;
    return (&result);
  }
  
  for (i = 0; i < NextSlot; i++) {
    if (!strcmp(KvStore[i][0], kv->key)) {
      fprintf(stdout, "PUT: key=\"%s\", value=\"%s\", old_value=\"%s\"\n",
	      kv->key, kv->value, KvStore[i][1]);
      strcpy(KvStore[i][1], kv->value);
      result = PUT_DUPLICATE_KEY_VALUE_RESET;
      return (&result);
    }
  }

  if (i == NextSlot) {
    fprintf(stdout, "PUT: key=\"%s\", value=\"%s\"\n",
            kv->key, kv->value);
    strcpy(KvStore[i][0], kv->key);
    strcpy(KvStore[i][1], kv->value);
    NextSlot++;
    result = PUT_OK;
    return (&result);
  }

  fprintf(stderr, "PUT: failed for unknown reason!\n");
  result = PUT_FAILED;
  return (&result);
}

GetReply * rget_1_svc(char **key, struct svc_req *req) {
  static GetReply result; /* must be static */
  static char buffer[MAX_LENGTH]; /* must be static */
  int i;

  if (result.value == NULL) {
    result.value = buffer;
  }

  strcpy(result.value, "");
  
  if ((key == NULL) || (*key == NULL)) {
    fprintf(stderr, "GET: key is null\n");
    result.code = GET_KEY_IS_NULL;
    return (&result);
  }

  if (strlen(*key) + 1 >= MAX_LENGTH) {
    fprintf(stderr, "GET: key is too large\n");
    result.code = GET_KEY_IS_TOO_LARGE;
    return (&result);
  }

  if (NextSlot == 0) {
    fprintf(stderr, "GET: key value store is empty\n");
    result.code = GET_KVSTORE_IS_EMPTY;
    return (&result);
  }

  for (i = 0; i < NextSlot; i++) {
    if (!strcmp(KvStore[i][0], *key)) {
      fprintf(stdout, "GET: key=\"%s\", value=\"%s\"\n", *key, KvStore[i][1]);
      strcpy(result.value, KvStore[i][1]);
      result.code = GET_OK;
      return (&result);
    }
  }

  if (i == NextSlot) {
    fprintf(stdout, "GET: key=\"%s\" not found\n", *key);
    result.code = GET_KEY_NOT_FOUND;
    return (&result);    
  }
  
  fprintf(stderr, "GET: failed for unknown reason!\n");
  result.code = GET_FAILED;
  return (&result);
}

int * rdel_1_svc(char **key, struct svc_req *req) {
  static int result; /* must be static */
  int i;

  if ((key == NULL) || (*key == NULL)) {
    fprintf(stderr, "DEL: key is null\n");
    result = DEL_KEY_IS_NULL;
    return (&result);
  }

  if (strlen(*key) + 1 >= MAX_LENGTH) {
    fprintf(stderr, "DEL: key is too large\n");
    result = DEL_KEY_IS_TOO_LARGE;
    return (&result);
  }

  if (NextSlot == 0) {
    fprintf(stderr, "DEL: key value store is empty\n");
    result = DEL_KVSTORE_IS_EMPTY;
    return (&result);
  }

  for (i = 0; i < NextSlot; i++) {
    if (!strcmp(KvStore[i][0], *key)) {
      fprintf(stdout, "DEL: key=\"%s\", value=\"%s\"\n", *key, KvStore[i][1]);

      //
      // Compaction.
      //

      if (i < (NextSlot - 1)) {
	strcpy(KvStore[i][0], KvStore[NextSlot - 1][0]);
	strcpy(KvStore[i][1], KvStore[NextSlot - 1][1]);
      }

      NextSlot--;
      result = DEL_OK;
      return (&result);
    }
  }

  if (i == NextSlot) {
    fprintf(stderr, "DEL: key=\"%s\" not found\n", *key);
    result = DEL_KEY_NOT_FOUND;
    return (&result);
  }

  fprintf(stderr, "DEL: failed for unknown reason!\n");
  result = DEL_FAILED;
  return (&result);
}

//
// This function will assess the viability of performing a PUT operation
// based on the provided input instead of actually performing the operation.
// It will either return PUT_VOTE_YES or PUT_VOTE_NO.
//

int * vote_put_1_svc(KeyValue *kv, struct svc_req *req) {
  static int result; /* must be static */
  int i;

  if (kv == NULL) {
    fprintf(stderr, "PUT VOTE: null key value pair\n");
    result = PUT_VOTE_NO;
    return (&result);
  }

  if (kv->key == NULL) {
    fprintf(stderr, "PUT VOTE: key is null\n");
    result = PUT_VOTE_NO;
    return (&result);
  }

  if (strlen(kv->key) + 1 >= MAX_LENGTH) {
    fprintf(stderr, "PUT VOTE: key is too large\n");
    result = PUT_VOTE_NO;
    return (&result);
  }

  if (kv->value == NULL) {
    fprintf(stderr, "PUT VOTE: value is null\n");
    result = PUT_VOTE_NO;
    return (&result);
  }

  if (strlen(kv->value) + 1 >= MAX_LENGTH) {
    fprintf(stderr, "PUT VOTE: value is too large\n");
    result = PUT_VOTE_NO;
    return (&result);
  }

  if (NextSlot == KVSTORE_CAPACITY) {
    fprintf(stderr, "PUT VOTE: key value store is out of capacity\n");
    result = PUT_VOTE_NO;
    return (&result);
  }
  
  fprintf(stderr, "PUT VOTE: YES\n");
  result = PUT_VOTE_YES;

  return (&result);
}

//
// This function will assess the viability of performing a DEL operation
// based on the provided input instead of actually performing the operation.
// It will either return DEL_VOTE_YES or DEL_VOTE_NO.
//

int * vote_del_1_svc(char **key, struct svc_req *req) {
  static int result; /* must be static */
  int i;

  if ((key == NULL) || (*key == NULL)) {
    fprintf(stderr, "DEL VOTE: key is null\n");
    result = DEL_VOTE_NO;
    return (&result);
  }

  if (strlen(*key) + 1 >= MAX_LENGTH) {
    fprintf(stderr, "DEL VOTE: key is too large\n");
    result = DEL_VOTE_NO;
    return (&result);
  }

  if (NextSlot == 0) {
    fprintf(stderr, "DEL VOTE: key value store is empty\n");
    result = DEL_VOTE_NO;
    return (&result);
  }

  for (i = 0; i < NextSlot; i++) {
    if (!strcmp(KvStore[i][0], *key)) {
      fprintf(stdout, "DEL VOTE: key=\"%s\", value=\"%s\"\n", *key, KvStore[i][1]);
      fprintf(stdout, "DEL VOTE: YES\n");
      result = DEL_VOTE_YES;
      return (&result);
    }
  }

  if (i == NextSlot) {
    fprintf(stderr, "DEL VOTE: key=\"%s\" not found\n", *key);
    result = DEL_VOTE_NO;
    return (&result);
  }

  fprintf(stderr, "DEL VOTE: failed for unknown reason!\n");
  result = DEL_VOTE_NO;
  return (&result);
}
