Required C files:
   kv_proc_r.c // for server
   rkv.c // for client

Required header file:
   kv_proto.h // server response codes

Required RPCL file:
   kv.x

Compile RPCL file:
Command:
   rpcgen kv.x
Generated H file:
   kv.h
Generated C files:
   kv_clnt.c // client stub
   kv_svc.c // server stub
   kv_xdr.c

Compile/build (kv_r.sh):
   cc -c kv_xdr.c
   cc -c kv_svc.c
   cc -c kv_clnt.c
   cc -c kv_proc_r.c
   cc -c rkv.c
   cc -o server kv_xdr.o kv_svc.o kv_proc_r.o kv_clnt.o -lnsl
   cc -o client kv_xdr.o kv_clnt.o rkv.o -lnsl

Script:
   kv_r.sh // to compile RPCL and build server and client

Other files:
   readme.txt // this file
   replica //list of ip addresses of other server instances (EXCLUSIVE of othe one it is hosted on)
