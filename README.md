# lymon db
Lymon is in-memory database which has SET (key/value) and PUBSUB model. It is non-volatile memory that stores dataa in memory. Lymon can be used on 1337 port after running.

# Usage
```
make
./src/lymon
```

Connection
```
nc 127.0.0.1 1337
key value model:
set first_key MSG
get first_key
lymon response: MSG
pubsub:
subscribe chann_name chann_name1 chann_name2 chann_name3
publish chann_name1 "msg for chann_name1"
publish chann_name2 "msg for chann_name2"
publish chann_name1 chann_name2 "broadcast messages for channels"
```
