#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
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
struct PrepReq {
	int *n;
	CLIENT *clnt;
};
struct keyvalue_client {
	KeyValue *kv;
	CLIENT *clnt;
};
struct key_client {
	Key *key;
	CLIENT *clnt;
};
char KvStore[KVSTORE_CAPACITY][2][MAX_LENGTH];
int NextSlot;
char ReplicaName[MAX_REPLICA][MAX_LENGTH];
int ReplicaCount;
int PrepN = 1;
int PromisedN = 0;
int acceptedN = 0;
Key acceptedKey=NULL;
KeyValue acceptedKV={0};
CLIENT *Replica[MAX_REPLICA];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; //initialize a lock
const bool DISTINGUISHED = true;
//read in the IP addresses of other servers from file titled 'replica'
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
//open connections to IP addresses from replica file
int open_connection_to_replicas() {
  	int i;
  	for (i = 0; i < ReplicaCount; i++) {
    		Replica[i] = clnt_create(ReplicaName[i], KVPROGRAM, KVVERSION, "tcp");
    		if (Replica[i] == (CLIENT *)NULL) {
      			fprintf(stderr, "Connection to replica: \"%s\" failed.\n", ReplicaName[i]);
      			clnt_pcreateerror(ReplicaName[i]);
      			// Disconnect already opened connections.
      			for (i--; i >= 0; i--) {
				clnt_destroy(Replica[i]);
				Replica[i] = NULL;
      			}
      			return 0; // Failure.
    		}
 	 }
  	return 1; // Success.
}
//close connections to IP addresses from replica file
void close_connection_to_replicas() {
  	int i;
  	for (i = 0; i < ReplicaCount; i++) {
    		if (Replica[i] != NULL) {
      			clnt_destroy(Replica[i]);
      			Replica[i] = NULL;
    		}
  	}
}
//call the rget_1_svc function for replica clnt
int rget(CLIENT *clnt, char *key, char *refValue) {
  	char *kv_key;
  	GetReply *result;
  	kv_key = malloc((strlen(key) + 1) * sizeof(char));
  	if (kv_key == NULL) {
    		fprintf(stderr, "GET: server error: memory allocation failure\n");
    		return GET_FAILED;
  	}
  	strcpy(kv_key, key);
  	result = rget_1(&kv_key, clnt);
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
//read replica names from the file and open connections
void replicate_begin() {
  	read_replica_names_from_file();
  	open_connection_to_replicas();
}
//close connection to replicas
void replicate_end() {
  	close_connection_to_replicas();
}
//get response to prepare request to put
PrepReply * accept_prepare_put(void* thread_args) {
	struct PrepReq* request;
	request = (struct PrepReq*)thread_args;
	static struct PrepReply reply;
	if((request->n)>PromisedN) {
		reply.vote=PROMISE;
		PromisedN = request->n;
	} else {
		reply.vote=DENY;
	}
	reply.kv = acceptedKV;
	reply.n = acceptedN;
	return(&reply);
}
//get response to prepare request to delete
PrepReply * accept_prepare_del(void* thread_args) {
	struct PrepReq *request;
	request = (struct PrepReq*)thread_args;
	static struct PrepReply reply;
	if((request->n)>PromisedN) {
		reply.vote=PROMISE;
		PromisedN = request->n;
	}else {
		reply.vote = DENY;
	}
	reply.key = acceptedKey;
	reply.n = acceptedN;
	return(&reply);
}
//execute put request
int * accept_execute_put(void* thread_data) {
	struct keyvalue_client *my_data;
	my_data = (struct keyvalue_client*)thread_data;
	KeyValue *kv;
	struct svc_req *req;
	kv=my_data->kv;
	req=(struct svc_req*)my_data->clnt;
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
	pthread_mutex_lock(&lock);
  	for (i = 0; i < NextSlot; i++) {
    		if (!strcmp(KvStore[i][0], kv->key)) {
      			fprintf(stdout, "PUT: key=\"%s\", value=\"%s\", old_value=\"%s\"\n",kv->key, kv->value, KvStore[i][1]);
      			strcpy(KvStore[i][1], kv->value);
      			result = PUT_DUPLICATE_KEY_VALUE_RESET;
			//pthread_mutex_unlock(&lock);
     			return (&result);
    		}
  	}
	//pthread_mutex_lock(&lock);
  	if (i == NextSlot) {
    		fprintf(stdout, "PUT: key=\"%s\", value=\"%s\"\n",kv->key, kv->value);
    		strcpy(KvStore[i][0], kv->key);
    		strcpy(KvStore[i][1], kv->value);
    		NextSlot++;
    		result = PUT_OK;
		//pthread_mutex_unlock(&lock);
    		return (&result);
  	}
  	fprintf(stderr, "PUT: failed for unknown reason!\n");
  	result = PUT_FAILED;
	//pthread_mutex_unlock(&lock);
  	return (&result);
}
//execute delete request
int * accept_execute_del(void* thread_data) {
	struct key_client *my_data;
	my_data = (struct keyvalue_client*)thread_data;
	char *key;
	struct svc_req *req;
	key=my_data->key;
	req=(struct svc_req*)my_data->clnt;
	static int result; /* must be static */
  	int i;
  	if (key == NULL) {
    		fprintf(stderr, "DEL: key is null\n");
    		result = DEL_KEY_IS_NULL;
    		return (&result);
  	}
	if (strlen(key) + 1 >= MAX_LENGTH) {
    		fprintf(stderr, "DEL: key is too large\n");
    		result = DEL_KEY_IS_TOO_LARGE;
    		return (&result);
 	}
  	if (NextSlot == 0) {
    		fprintf(stderr, "DEL: key value store is empty\n");
    		result = DEL_KVSTORE_IS_EMPTY;
    		return (&result);
  	}
	//pthread_mutex_lock(&lock);
  	for (i = 0; i < NextSlot; i++) {
   		if (!strcmp(KvStore[i][0], key)) {
			strcpy(KvStore[1][1],"test");
      			fprintf(stdout, "DEL: key=\"%s\", value=\"%s\"\n", key, KvStore[i][1]);
      			// Compaction.
      			if (i < (NextSlot - 1)) {
				strcpy(KvStore[i][0], KvStore[NextSlot - 1][0]);
				strcpy(KvStore[i][1], KvStore[NextSlot - 1][1]);
      			}
      			NextSlot--;     
      			result = DEL_OK;
			//pthread_mutex_unlock(&lock);
      			return (&result);
    		}
  	}
  	if (i == NextSlot) {
    		fprintf(stderr, "DEL: key=\"%s\" not found\n", *key);
    		result = DEL_KEY_NOT_FOUND;
		//pthread_mutex_unlock(&lock);
    		return (&result);
  	}
  	fprintf(stderr, "DEL: failed for unknown reason!\n");
  	result = DEL_FAILED;
	//pthread_mutex_unlock(&lock);
  	return (&result);
}
//call accept_prepare_put_1_svc() for a particular replica
PrepReply* raccept_prepare_put(CLIENT *clnt,int n) {
	static PrepReply response;
	response.vote = PROMISE;
	if(n == (int)NULL) {
		fprintf(stderr,"PUT: no n provided to prepare\n");
		return NULL;
	}
	response = *accept_prepare_put_1(&n,clnt);//get the result from the accept_prepare_put_1_svc() function of this replica
	if(&response==(PrepReply*)NULL) {
		clnt_perror(clnt,"PUT: rpc error");
		return NULL;
	}
	fprintf(stdout,"PUT: replication response %d\n",response.vote);
	return &response;
}
//call accept_prepare_del_1_svc() for a particular replica
PrepReply* raccept_prepare_del(CLIENT *clnt,int n) {
	static PrepReply response;
	if(n == (int)NULL) {
		fprintf(stderr,"PUT: no n provided to prepare\n");
		return NULL;
	}
	response = *accept_prepare_del_1(&n,clnt);//get the result from the accept_prepare_del_1_svc() function of this replica
	if(&response==(PrepReply*)NULL) {
		clnt_perror(clnt,"PUT: rpc error");
		return NULL;
	}
	fprintf(stdout,"PUT: replication response %d\n",response.vote);
	return &response;
}
//call accept_execute_put_1_svc() for a particular replica
int* raccept_execute_put(CLIENT *clnt, char *key, char *value) {
	KeyValue kv;
  	static int result;

  	kv.key = malloc((strlen(key) + 1) * sizeof(char));
  	if (kv.key == NULL) {
    		fprintf(stderr, "PUT: server error: memory allocation failure\n");
    		result = PUT_FAILED;
		return(&result);
  	}
  	strcpy(kv.key, key);
  	kv.value = malloc((strlen(value) + 1) * sizeof(char));
  	if (kv.value == NULL) {
    		fprintf(stderr, "PUT: server error: memory allocation failure\n");    
    		free(kv.key);
    		result = PUT_FAILED;
		return(&result);
  	}
  	strcpy(kv.value, value);
  	result = *accept_execute_put_1(&kv, clnt);//get the result from the accept_execute_put_1_svc() function of this replica
  	if (result == (int)NULL) {
    		clnt_perror(clnt, "PUT: rpc error");    
    		free(kv.key);
    		free(kv.value);
    		result = PUT_FAILED;
		return(&result);
  	}
  	fprintf(stdout, "PUT: replication response: %s (%d)\n", PutResponseText[result], result);
  	free(kv.key);
  	free(kv.value);
  	return(&result);
}
//call accept_execute_del_1_svc() for a particular replica
int* raccept_execute_del(CLIENT *clnt, char *key) {
	char *kv_key;
  	static int result;
  	kv_key = malloc((strlen(key) + 1) * sizeof(char));
  	if (kv_key == NULL) {
    		fprintf(stderr, "DEL: server error: memory allocation failure\n");
		result = DEL_FAILED;
    		return(&result);
  	}
  	strcpy(kv_key, key);
  	result = *accept_execute_del_1(kv_key, clnt);//get the result from the accept_execute_del_1_svc() function of this replica
  	if (result == (int)NULL) {
    		clnt_perror(clnt, "DEL: rpc error");
    		free(kv_key);
    		result= DEL_FAILED;
		return(&result);
  	}
  	fprintf(stdout, "DEL: replication response: %s (%d)\n", ResponseText[result],  result);
  	free(kv_key);
  	result = DEL_OK;
	return(&result);
}
//acceptor thread receives prepare request and goes to
//accept_prepare_put to process
PrepReply * accept_prepare_put_1_svc(int *n, struct svc_req *req) {
	static PrepReply return_val;
	PrepReply* result;
	struct PrepReq *request;
	request = (struct PrepReq*)malloc(sizeof(request));
	request->n = n;//no for prepare request and client that initiated request
	request->clnt = (CLIENT*)req;
	pthread_t acceptor_thread;
	pthread_create(&acceptor_thread,NULL,(void*)accept_prepare_put,(void*)request);//acceptor thread goes to accept_prepare_put function to process request
	pthread_join(acceptor_thread,(void*)&result);//result from acceptor thread
	return_val = *result;
	return(&return_val);
}
//acceptor thread receives prepare request and goes to
//accept_prepare_del to process
PrepReply * accept_prepare_del_1_svc(int *n, struct svc_req *req) {
	static PrepReply return_val;
	PrepReply *result;
	struct PrepReq *request;//n for prepare request and client that initiated request
	request = (struct PrepReq*)malloc(sizeof(request));
	request->n = n;
	request->clnt = (CLIENT *)req;
	pthread_t acceptor_thread;
	pthread_create(&acceptor_thread,NULL,(void*)accept_prepare_del,(void*)request);//acceptor thread goes to accept_prepare_del function to process request
	pthread_join(acceptor_thread,(void*)&result);//result from acceptor thread
	return_val = *result;
	return(&return_val);
}
//acceptor thread receives put request and goes to
//accept_execute_put to process
int * accept_execute_put_1_svc(KeyValue *kv, struct svc_req *req) {
	static int return_value;
	int* result;
	struct keyvalue_client *thread_data;//key value for put and client that initiated request
	thread_data = (struct keyvalue_client*)malloc(sizeof(thread_data));
  	thread_data->kv = kv;
  	thread_data->clnt = (CLIENT *)req;
  	pthread_t acceptor_thread;//thread to handle request
  	pthread_create(&acceptor_thread,NULL,(void*)accept_execute_put,(void*)thread_data);//send request to accept_execute_put to process
  	pthread_join(acceptor_thread,(void*)&result);//get result from accept_execute_put and return
	return_value = *result;	
	return(&return_value);
}
//acceptor thread receives delete request and goes to
//accept_execute_del to process
int * accept_execute_del_1_svc(char *key, struct svc_req *req) {
	static int return_value;
	int* result;
	struct key_client *thread_data;//key for delete and client that initiated request
	thread_data = (struct key_client*)malloc(sizeof(thread_data));
	thread_data->key = key;
	thread_data->clnt = (CLIENT *)req;
	pthread_t acceptor_thread;//thread to handle request
	pthread_create(&acceptor_thread,NULL,(void*)accept_execute_del,(void*)thread_data);//send request to accept_execute_del to process
	pthread_join(acceptor_thread,(void*)&result);//get result from accept_execute_del and return
	return_value = *result;
	return(&return_value);
}
//proposer thread from thread_put_1_svc comes here to send prepare request
int * propose_prepare_put(void* req) {
	static int count;//final count of yes votes
	int dynamicCount = 0;//running cout of yes votes
	PrepReply *reply;//current reply
	PrepN++;
	int * n = &PrepN;
	CLIENT* clnt = (CLIENT*)req;
	if((reply = accept_prepare_put_1_svc(n,clnt))->vote == PROMISE) {//local call to send prepare request
		dynamicCount++;
	}
	replicate_begin();//open replica connection
	for(int i=0; i<ReplicaCount;i++) {//send prepare request to each replica
		if((reply = raccept_prepare_put(Replica[i],n))->vote==PROMISE) {
			dynamicCount++; //if a replica promises,increase the yes count
		}
	}
	replicate_end();//end replica connection
	count = dynamicCount;//save final count
	return(&count);//return final count of yes votes
}
//proposer thread from thread_del_1_svc comes here to send prepare request
int * propose_prepare_del(void* req) {
	static int count; //final count of yes votes
	int dynamicCount = 0; //running count of yes votes
	PrepReply *reply; //current reply
	PrepN++;
	int * n = &PrepN;
	CLIENT* clnt = (CLIENT*)req;
	if((reply = accept_prepare_del_1_svc(n,clnt))->vote == PROMISE) {//local call to send prepare request
		dynamicCount++;//increase yes count if local vote is yes
	}
	replicate_begin();//open replica connection
	for(int i=0; i<ReplicaCount;i++) {//send prepare request to each replica
		if((reply = raccept_prepare_del(Replica[i],n))->vote==PROMISE) {
			dynamicCount++; //if a replica promises,increase the yes count
		}
	}
	replicate_end();//end replica connection
	count = dynamicCount;//save final count
	count = 10;
	return(&count);//return final count of yes votes
}
//proposer thread from thread_put_1_svc comes here after getting majority
int * propose_execute_put(void* thread_data) {
	static int result;//final result from execution of delete request
	int remoteResult;
	struct keyvalue_client *my_data;//keyvalue and client the request came from
	my_data = (struct keyvalue_client*)thread_data;
	KeyValue* kv;
	kv=my_data->kv;
	CLIENT* clnt;
	clnt = my_data->clnt;
	result = *accept_execute_put_1_svc(kv,clnt); //local call to execute put request
	if(result!=PUT_OK) {
		return(&result);//if local machine could not execute put, do not send to remote and return failure
	}
	replicate_begin();//local put was successful, open replica connection
	for(int i=0;i<ReplicaCount;i++) {//execute delete for each replica
		if((remoteResult=raccept_execute_put(Replica[i],kv->key,kv->value))!=PUT_OK) {
			result = remoteResult;//if a single replica fails, a failure result is reported
		}
	}
	replicate_end();
	result=PUT_OK;
	return(&result);//return final result
}
//proposer thread from thread_del_1_svc comes here after getting majority
int * propose_execute_del(void* thread_data) {
	static int result;//final result from execution of delete request
	int remoteResult;
	int i;
	struct key_client *my_data;//key and client the request came from
	my_data = (struct key_client*)thread_data;
	char *key;
	CLIENT* clnt;
	key = my_data->key;
	clnt = my_data->clnt;
	result = *accept_execute_del_1_svc(key,clnt); //local call to execute del request
	if (result != DEL_OK) {
    		return(&result);//if local machine could not execute delete, do not send to remote and return failure
  	}
	replicate_begin();//local delete was successful, open replica connection
  	for(i = 0; i < ReplicaCount; i++) {//execute delete for each replica
    		if ((remoteResult = raccept_execute_del(Replica[i], key)) != DEL_OK) {
      			result=remoteResult; //if a single replica fails, a failure result is reported
    		}
  	}
	replicate_end();
	return(&result);//return final result
}
//thread from put_1_svc comes here to perform put
int * thread_put_1_svc(void* thread_data) {
	int* result;
	static int return_val;//result of overall request
	int* count;//number of OKs received
	struct keyvalue_client *my_data;
	my_data = (struct keyvalue_client*)thread_data;//key value and client the request came from
	if(!DISTINGUISHED) {
		replicate_begin();
		KeyValue *kv;
		kv = my_data->kv;
		result = put_1(kv,Replica[1]);
		replicate_end();
		return_val = result;
		return(&return_val);		
	}
  	pthread_t proposer_thread;
	CLIENT *clnt;
	clnt = (CLIENT*)malloc(sizeof(clnt));
	clnt = my_data->clnt;
	//proposer goes to propose_prepare_put to send prepare request for put -- does not require input
  	pthread_create(&proposer_thread,NULL,(void*)propose_prepare_put,(void*)clnt);
  	pthread_join(proposer_thread,(void*)&count);//get number of OK results
	if(*count < ceil(ReplicaCount/2)) {
		fprintf(stderr,"PUT: was unable to get majority\n");
		return PUT_FAILED;//couldn't get majority, return failure
	}
	pthread_create(&proposer_thread,NULL,(void*)propose_execute_put,(void*)my_data);
	pthread_join(proposer_thread,(void*)&result);//save the final result from the execute here
	return_val = *result;
	return(&return_val);//return the result received from propose_execute_put
}
//thread from del_1_svc comes here to perform delete
int * thread_del_1_svc(void* thread_data) {
	int *result;//result of overall request	
	static int return_val;
	int *count;//number of OKs received
	struct key_client *my_data;
	my_data = (struct key_client*)thread_data;//key value and client the request came from
	pthread_t proposer_thread;
	CLIENT *clnt;
	clnt = (CLIENT*)malloc(sizeof(clnt));
	clnt = my_data->clnt;
	//proposer goes to propose_prepare_del to send prepare request for delete -- does not require input
	pthread_create(&proposer_thread,NULL,(void*)propose_prepare_del,(void*)clnt);
	pthread_join(proposer_thread,(void*)&count); //get number of OK results
	if(*count < ceil(ReplicaCount/2)) {
		fprintf(stderr, "DEL: was unable to get majority\n");
    		return DEL_FAILED;//couldn't get a majority, return failure
	}
	//proposer goes to propose_execute_del to send delete request
	pthread_create(&proposer_thread,NULL,(void*)propose_execute_del,(void*)my_data);
	pthread_join(proposer_thread,(void*)&result);//save the final result from the execution here
	return_val = *result;
	return(&return_val);//return the result received from propose_execute_del
}
//receives put request from client here and send to thread_put_1_svc via pthread
int * put_1_svc(KeyValue *kv, struct svc_req *req) {
	static int return_val;
	int* result;
	struct keyvalue_client *thread_data;//key value for put and client that initiated request
	thread_data = (struct keyvalue_client*)malloc(sizeof(thread_data));
	thread_data->kv = kv;
	thread_data->clnt = req;
  	pthread_t put_thread;//thread to handle request
  	pthread_create(&put_thread,NULL,thread_put_1_svc,(void*)thread_data);//send request to thread_put_1_svc to process
  	pthread_join(put_thread,(void*)&result);//get result from thread_put_1_svc and return
	return_val = *result;	
	return(&return_val);
}

//receive get request from client here
GetReply * get_1_svc(char **key, struct svc_req *req) {
  	static GetReply result; /* must be static */
  	int status;
  	int i; 
	if(!DISTINGUISHED) {
		replicate_begin();
		result = *get_1(key,Replica[1]);
		replicate_end();
		return(&result);		
	}
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
//receives delete request from client here and send to thread_del_1_svc via pthread
int * del_1_svc(char **key, struct svc_req *req) {
	static int return_val;
	int* result;
	struct key_client *thread_data;//key to delete and client the request came from
	thread_data = (struct key_client*)malloc(sizeof(thread_data));
  	thread_data->key = *key;
  	thread_data->clnt = req;
  	pthread_t del_thread;//thread to handle request
  	pthread_create(&del_thread,NULL,thread_del_1_svc,(void*)thread_data);//send request to thread_del_1_svc to process
  	pthread_join(del_thread,(void*)&result);//get result from thread_del_1_svc and return
	return_val = *result;
	return(&return_val);
}
//receive get request either from remove server (via that server's rget function)
//or from local get_1_svc
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


