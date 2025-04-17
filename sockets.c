
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#define SOCKET int
#define ISVALIDSOCKET(s) ((s) >= 0)

int
main()
{
	printf("Configuring local address...\n");

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *bind_address;
	int res = getaddrinfo(0, "8080", &hints, &bind_address);
	if (res) {
		fprintf(stderr, "could not get address info: (%d): %s\n", res, gai_strerror(res));
		return res;
	}

	printf("Creating socket...\n");
	SOCKET socket_listen = socket(
		bind_address->ai_family,
		bind_address->ai_socktype,
		bind_address->ai_protocol
	);
	if (socket_listen < 0) {
		fprintf(stderr, "socket() failed. (%d)\n", errno);
		return errno;
	}

	printf("Binding socket to local address...\n");
	if (bind(
		socket_listen,
		bind_address->ai_addr,
		bind_address->ai_addrlen
	)) {
		fprintf(stderr, "bind() failed. (%d)\n", errno);
		freeaddrinfo(bind_address);
		return errno;
	}
	freeaddrinfo(bind_address);

	printf("Listening...\n");
	if (listen(socket_listen, 10) < 0) {
		fprintf(stderr, "listen() failed. (%d)\n", errno);
		return errno;
	}

	printf("Waiting for connection...\n");
	struct sockaddr_storage client_address;
	socklen_t client_len = sizeof(client_address);
	SOCKET socket_client = accept(
		socket_listen,
		(struct sockaddr*) &client_address,
		&client_len
	);
	if (socket_client < 0) {
		fprintf(stderr, "accept() failed. (%d)\n", errno);
		return errno;
	}

	printf("Client is connected...\n");
	char address_buffer[100];
	getnameinfo(
		(struct sockaddr*) &client_address,
		client_len,
		address_buffer,
		sizeof(address_buffer), 0, 0,
		NI_NUMERICHOST
	);
	printf("%s\n", address_buffer);

	printf("Reading request..\n");
	char request[1024];
	int bytes_received = recv(socket_client, request, 1024, 0);
	printf("Received %d bytes.\n", bytes_received);

	printf("Sending response...\n");
	const char *response =
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"Local time is: ";
	int bytes_sent = send(
		socket_client,
		response,
		strlen(response), 0
	);
	printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));


	time_t timer;
	time(&timer);
	char *time_msg = ctime(&timer);
	bytes_sent = send(
		socket_client,
		time_msg,
		strlen(time_msg), 0
	);
	printf("Sent %d bytes.\n", bytes_sent);

	printf("Closing connection...\n");
	close(socket_client);

	printf("Finished.\n");

	return 0;
}

