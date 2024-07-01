
# lock-manager - how to use

## basics

to use the lock-manager, the lock-manager daemon must be started with lock-manager -s.
note - the locks that are created and released MUST have unique names. if you try to aquire a lock with the same name as one that already exists, it will fail.
return codes - when you run the command, it will return either 0 for success or 1 for failure. for example:
`lock-manager -a "lock 1"` will return 0 if it is able to create the lock, or 1 if the lock already exists.
`lock-manager -r "lock 1"` will return 0 if it was able to release the lock, or 1 if the lock did not already exist.
`lock-manager -q lock 1` will return 0 if the lock exists, and 1 if it does not.

### commands

> 1. `acquire : -a` this creates a 
