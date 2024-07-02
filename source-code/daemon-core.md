
# How to use `daemon-core.c` and coresponding `daemon-core.h`

## Sockets

### `int make_named_socket(const char *filename)`

### `int connect_named_socket(const char *filename)`

### `int close_named_socket(int socket, const char *filename)`

### `int send_string(int socket, const char *string)`

### `char *receive_string(int socket)`

## Locks 

### `struct locks`

### `void free_locks()`

# How it works

## `start_daemon()`

## `acquire_lock()`

## `query_lock()`

## `release_lock()`
