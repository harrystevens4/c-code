#ifndef _QD_H
#define _QD_H

/*
Here is the standard
See all relevant structs and enums below

=== filtering ===

setting the hostname and/or service in the discover
will cause only servers with matching hostnames or services
to respond. setting the service to all '\0's will cause the service to be
ignored, and setting the hostname length to 0 will cause the hostname to be ignored

=== only one qd service running ===

client                      server
   |  qd_discover_packet      |
   |------------------------->|
   |                          |
   |  qd_response_packet      |
   |<-------------------------|
   |
   |
   \
   clients ip is {w.x.y.z}

   responses will only come from servers
   with matching hostnames to that set
   in the discover packet

=== multiple qd services running on one server ===

client                           server
   |  qd_discover_packet           |
   | {hostname = "desktop-abc"}    |
   |------------------------------>|
   |                               |
   |                               |\
   |       qd_response_packet      | |\
   |<------------------------------| | |
   |       qd_response_packet      | | |
   |<--------------------------------| |
   |       qd_repsonse_packet      | | |
   |<----------------------------------|
   |                               | | |
   |                               | |  \
   |                               | |   service 3
   |                               |  \
   |                               |   service 2
   |                                \
   \                                 service 1
   `desktop-abc` ip is {w.x.y.z}; service `TIME`
   `desktop-abc` ip is {w.x.y.z}; service `PWRSWTCH`
   `desktop-abc` ip is {w.x.y.z}; service `BUZZER`

   qd_recv_response() will receive one response

   calling qd_recv_response() repeatedly untill a timeout
   or untill you find a server with the correct service
   is a great way to use this.
   dont forget you can use poll() on the qd file descriptor
   to see if there is a response pending.

=== multiple qd services running on different servers

                                    "desktop-a"
                                    / "desktop-b" has service TIME
                                   |  / "desktop-c"
client                             | |  / "desktop-d" has service TIME
   |  qd_discover_packet           | | |  /
   |   {hostname_len = 0,          | | | |
   |   service = "TIME"}           | | | |
   |------------------------------>+-+-+-+
   |                               | | | |
   |       qd_response_packet      | | | |
   |<--------------------------------| | |
   |       qd_response_packet      | | | |
   |<------------------------------------|
   |                               | | | |
   |                               | | | |
   \
   `desktop-b` ip is {w.x.y.z}; service `TIME`
   `desktop-d` ip is {w.x.y.z}; service `TIME`

   setting the `hostname_len` to 0 will cause
   all available services on all available devices
   to respond, but setting the service here will
   mean only those services on those devices will

*/

#define SERVICE_LEN 12
#define HOSTNAME_MAX_LEN 254
//all fields are stored in network byte order (obviously)
struct __attribute__((packed)) qd_discover_packet {
	char hostname[HOSTNAME_MAX_LEN]; //not null terminated
	uint8_t hostname_len;
	char service[SERVICE_LEN]; //not null terminated but the rest of the buffer should be '\0'
};
struct __attribute__((packed)) qd_response_packet {
	char hostname[HOSTNAME_MAX_LEN];
	uint8_t hostname_len;
	char service[SERVICE_LEN]; //small string to identify what service is being offered
	struct sockaddr_storage address;
};


#endif
