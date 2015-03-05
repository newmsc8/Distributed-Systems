
#define PROGRAM_NUMBER 123456
#define VERSION_NUMBER 1

typedef string Key<>;
typedef string Value<>;

struct KeyValue {
  Key key;
  Value value;
};

struct GetReply {
  int code;
  Value value;
};

struct PrepReply {
	int vote;
	KeyValue kv;
	Key key;
	int n;
};

program KVPROGRAM {
  version KVVERSION {
    int PUT(KeyValue) = 1;
    GetReply GET(string) = 2;
    int DEL(string) = 3;

    GetReply RGET(string) = 4;

    PrepReply ACCEPT_PREPARE_DEL(int) = 5;
    PrepReply ACCEPT_PREPARE_PUT(int) = 6;
    int ACCEPT_EXECUTE_DEL(char) = 7;
    int ACCEPT_EXECUTE_PUT(KeyValue) = 8;
  } = VERSION_NUMBER;
} = PROGRAM_NUMBER;
