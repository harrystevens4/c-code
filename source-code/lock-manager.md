
# lock-manager - how to use

## basics

To use the lock-manager, the lock-manager daemon must be started with `lock-manager -s`.
Note - the locks that are created and released MUST have unique names. if you try to aquire a lock with the same name as one that already exists, it will fail.
Return codes - when you run the command, it will return either 0 for success or 1 for failure.

### examples:  

`lock-manager -a "lock 1"` will return 0 if it is able to create the lock, or 1 if the lock already exists.  
`lock-manager -r "lock 1"` will return 0 if it was able to release the lock, or 1 if the lock did not already exist.  
`lock-manager -q lock 1` will return 1 if the lock exists, and 0 if it does not.

## commands

1. `acquire : -a` This creates a lock with the specified name
2. `release : -r` This releases a lock with the specified name
3. `query : -q` This checks the status of a lock (exists or not)

## workings

### locks

The locks themselves are stored in a struct as an array of strings containing each locks name.  
When creating a lock, the daemon will append the name of the lock to the end of the list.  
When querying the lock, the daemon will itterate through and check if the list contains the named entry.  
When removing a lock, the daemon will replace the index of the stored lock with the last lock of the array, and shrink the array by one element.
	swap last element and one to replace:

	lock 1 | lock 2 | lock 3 | lock 4
	            |                |
	            +--------<-------+
	
	shrink array by 1:

	+--------------------------+ <--
	| lock 1 | lock 4 | lock 3 | lock 4
	+--------------------------+ <---
	
	final layout:

	lock 1 | lock 4 | lock 3
