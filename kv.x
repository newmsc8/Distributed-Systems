
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

program KVPROGRAM {
  version KVVERSION {
    int PUT(KeyValue) = 1;
    GetReply GET(string) = 2;
    int DEL(string) = 3;
  } = VERSION_NUMBER;
} = PROGRAM_NUMBER;
