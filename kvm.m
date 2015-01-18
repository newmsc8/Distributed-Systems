#include "kvm.h"
char* keyValueString;
char** keyValues;
int count;

int loadKeyValueString(){
	keyValueString = malloc(0);
	int totalSize = 0;
	int i = 0;

	FILE *fp;
	char c;
	fp = fopen("keyValue","r");
	if(fp == NULL) 
	{
		return(-1);
	}
	do
	{
		c = fgetc(fp);
		if( feof(fp) )
		{
			break ;
		}
		if(i >= totalSize){
			char tmp[i];
			strcpy(tmp,keyValueString);
			keyValueString = malloc(1000+i);
			strcpy(keyValueString,tmp);
		}
		keyValueString[i] = c;
		i++;
		keyValueString[i] = '\0';

	}while(1);
	fclose(fp);
	
	return 0;
}
int arrycpy(char** array1, char** array2,int count){
	for(int i = 0; i < count;i++){
		array1[i] = malloc(strlen(array2[i]));
		strcpy(array1[i],array2[i]);
	}
	return 0;
}
int printStinrgArray(char** array,int c){
	printf("[");
	for(int i = 0; i< c; i++){
		//printf("%d\n", c);
		printf("%s, ",array[i]);
	}
	printf("]");
	return 0;
}
int size(int* i){
	char* token = strtok(keyValueString,"\n");
	  while (token != NULL){
	  	i++;
	  	token = strtok (NULL, "\n");
	  }
	return 0;
}
int insert(char* value){
	if(count*4 <= sizeof(keyValues)){
		char* tmp[count];
		arrycpy(tmp,keyValues,count);
		keyValues = malloc(8000+count);
		arrycpy(keyValues,tmp,count);
	}
	keyValues[count] = malloc(strlen(value));
	strcpy(keyValues[count],value);
	count ++;
	return 0 ;
}
int parseKeyValueString(){
	keyValues = malloc(0);
	count = 0;
	char* token = strtok(keyValueString,"\n");
	  while (token != NULL){
	  	insert(token);
	  	token = strtok (NULL, "\n");
	  }
	return 0 ;
}
int isEmpty(char* string){
	for(int i = 0; i < strlen(string);i++){
		if(string[i] != '\0'){
			return 0;
		}
	}
	return 1;
}
int get(char* key, char* value){
	for(int i = 0; i < count; i++){
		char str [strlen(keyValues[i])];
		strcpy(str,keyValues[i]);
		char* token = strtok(str,":");
		if (strcmp(key, token) == 0)
		{
			token = strtok (NULL, ":");
			if (token == NULL || isEmpty(token) == 1 )
			{
				return NULL_VALUE_ERR;
			}
			strcpy(value, token);
			return i;
		}
	}
	return UNKNOWN_ERR;
}
int saveChagnes(){
	FILE *f = fopen("keyValue","w");
	if (f == NULL){
		return FILE_ERR;
	}
	for(int i = 0; i< count; i++){
		char* val = keyValues[i];
		printf("%s\n", val);
		if (isEmpty(val) == 0)
		{
			fprintf(f, "%s\n",val);
		}
	}
	fclose(f);
	loadKeyValueString();
	parseKeyValueString();
	return 0;
}
int put(char* key, char* value){
	char val[100];
	int i = get(key,val);
	if(i >= 0){
		printStinrgArray(keyValues,count);
		return DUPLICATE_KEY_ERR;
	}
	if(isEmpty(key) == 0 && isEmpty(value) == 0){
		char str[strlen(key)+1+strlen(value)];
		strcpy(str, key);
		strcat(str,":");
		strcat(str,value);
		insert(str);
		saveChagnes();
		return 0;
	}
	return KEY_VALUE_ERR;
}
int delete_key(char* key){
	if(isEmpty(key) == 1){
		return NULL_KEY_ERR;
	}
	char val[100];
	int i = get(key,val);
	if(i >= 0){
		keyValues[i] = malloc(1);
		strcpy(keyValues[i],"\0");
	}
	saveChagnes();
	return 0;
}
void init_key_values(){
	loadKeyValueString();
	parseKeyValueString();
}
void lowercase(char* str){
	for(int i = 0; i < strlen(str);i++){
		str[i] = tolower(str[i]);
		i++; 
	}
}
int parse_package(char* package, char* results[]){
	char p[strlen(package)];
	strcpy(p, package);
	int i = 0;
	char* token = strtok(p,",");
	  while (token != NULL){
	  	//printf("%s\n",token );
	  	strcpy(results[i],token);
	  	token = strtok (NULL, ",");
	  	i++;
	  }
	  lowercase(results[0]);
	  if (strcmp(results[0], "put") == 0){
	  	return PUT;
	  }else if( strcmp(results[0],"get") == 0){
	  	return GET;
	  }else if(strcmp(results[0],"delete")== 0){
	  	return DELETE;
	  }
	  return MALFORMED_PACKET;

}
/*
void package(char* instr,int param, char* params[], char* packet){
	lowercase(instr);
	if(strcmp(instr, "put") == 0 && param = 2){
		strcpy(packet,instr);
		strcat(packet,",");
		strcat(packet,params[0]);
		strcat(packet,",");
		strcat(params[1]);
	}
}*/
int m(){
	loadKeyValueString();
	parseKeyValueString();
	printStinrgArray(keyValues,count);
	return 0;
}