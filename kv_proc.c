#include <stdio.h>
#include <pthread.h>
#include "kv.h"
#include "kv_proto.h"

char KvStore[KVSTORE_CAPACITY][2][MAX_LENGTH];
int NextSlot;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // initialize lock statically at declaration

int * put_1_svc(KeyValue *kv, struct svc_req *req) {
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

  // passed all of our input validation checks -- will now get lock before preceeding to CS
  pthread_mutex_lock(&lock);
  
  for (i = 0; i < NextSlot; i++) {
    if (!strcmp(KvStore[i][0], kv->key)) {
      fprintf(stdout, "PUT: key=\"%s\", value=\"%s\", old_value=\"%s\"\n",
	      kv->key, kv->value, KvStore[i][1]);
      strcpy(KvStore[i][1], kv->value);
      // completed action in CS and releases lock
      pthread_mutex_unlock(&lock);
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
    // completed action in CS and releases lock
    pthread_mutex_unlock(&lock);
    result = PUT_OK;
    return (&result);
  }

  // was unable to complete an action and releases lock
  pthread_mutex_unlock(&lock);
  fprintf(stderr, "PUT: failed for unknown reason!\n");
  result = PUT_FAILED;
  return (&result);
}

GetReply * get_1_svc(char **key, struct svc_req *req) {
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

int * del_1_svc(char **key, struct svc_req *req) {
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

  // passed all of our input validation checks -- will now get lock before preceeding to CS
  pthread_mutex_lock(&lock);

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
      // completed action in CS and releases lock
      pthread_mutex_unlock(&lock);
      result = DEL_OK;
      return (&result);
    }
  }

  if (i == NextSlot) {
    fprintf(stderr, "DEL: key=\"%s\" not found\n", *key);
    // was unable to complete an action and releases lock
    pthread_mutex_unlock(&lock);
    result = DEL_KEY_NOT_FOUND;
    return (&result);
  }

  fprintf(stderr, "DEL: failed for unknown reason!\n");
  // was unable to complete an action and releases lock
  pthread_mutex_unlock(&lock);
  result = DEL_FAILED;
  return (&result);
}
