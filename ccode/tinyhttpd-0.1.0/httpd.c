#include "httpd.h"

void bad_request(int client) {
	send(client, BADSTRING, strlen(BADSTRING), 0);
}

void cannot_execute(int client) {
	send(client, INTERNAL_SERVER_ERROR_STRING, strlen(INTERNAL_SERVER_ERROR_STRING), 0);
}

void unimplemented(int client) {
	send(client, METHOD_NOT_FOUND_STRING , strlen(METHOD_NOT_FOUND_STRING), 0);
}

void not_found(int client) {
	send(client, NOT_FOUND_STRING, strlen(NOT_FOUND_STRING), 0);
}

void error_die(const char *sc) {
	printf("error happend process died ... ");
	perror(sc);
	exit(1);
}

void accept_request(int *transfterclient) {
	int client = *transfterclient;
	char buf[1024];
	int numchars;
	char method[255];
	char url[255];
	char path[512];
	size_t i, j;
	struct stat st;
	int cgi = 0;      
	char *query_string = NULL;

	numchars = get_line(client, buf, sizeof(buf));
	i = 0; 
	j = 0;
	while (!ISspace(buf[j]) && (i < sizeof(method) - 1)) {
		method[i] = buf[j];
		i++; 
		j++;
	}
	method[i] = '\0';

	if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
		unimplemented(client);
		return;
	}

	if (strcasecmp(method, "POST") == 0) {
		cgi = 1;
	} 

	i = 0;

	while (ISspace(buf[j]) && (j < sizeof(buf))){
		j++;
	}

	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf))) {
		url[i] = buf[j];
		i++; j++;
	}
	url[i] = '\0';

	if (strcasecmp(method, "GET") == 0) {
		query_string = url;
		while ((*query_string != '?') && (*query_string != '\0')) {
			query_string++;
		}

		if (*query_string == '?') {
			cgi = 1;
			*query_string = '\0';
			query_string++;
		}
	}

	sprintf(path, "htdocs%s", url);
	if (path[strlen(path) - 1] == '/') {
		strcat(path, "index.html");
	}

	if (stat(path, &st) == -1) {
		while ((numchars > 0) && strcmp("\n", buf))  {
			numchars = get_line(client, buf, sizeof(buf));
		}
		not_found(client);
	} else {
		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			strcat(path, "/index.html");
		}
		if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH) ) {
			cgi = 1;
		}

		if (!cgi) {
			serve_file(client, path);
		} else {
			execute_cgi(client, path, method, query_string);
		}
	}
	close(client);
}

void cat(int client, FILE *resource) {
	char buf[1024];
	do {
		fgets(buf, 1024, resource);
		send(client, buf, strlen(buf), 0);
		bzero(buf,1024);
	} while(!feof(resource));
}

void execute_cgi(int client, const char *path, const char *method, const char *query_string) {
	char buf[1024];
	int cgi_output[2];
	int cgi_input[2];
	pid_t pid;
	int status;
	int i;
	char c;
	int numchars = 1;
	int content_length = -1;

	buf[0] = 'A'; buf[1] = '\0';
	if (strcasecmp(method, "GET") == 0) {
		while ((numchars > 0) && strcmp("\n", buf))  {
			numchars = get_line(client, buf, sizeof(buf));
		}
	} else {
		numchars = get_line(client, buf, sizeof(buf));
		while ((numchars > 0) && strcmp("\n", buf)) {
			buf[15] = '\0';
			if (strcasecmp(buf, "Content-Length:") == 0) {
				content_length = atoi(&(buf[16]));
			}
			numchars = get_line(client, buf, sizeof(buf));
		}
		if (content_length == -1) {
			bad_request(client);
			return;
		}
	}

	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);

	if (pipe(cgi_output) < 0) {
		cannot_execute(client);
		return;
	}

	if (pipe(cgi_input) < 0) {
		cannot_execute(client);
		return;
	}

	if ( (pid = fork()) < 0 ) {
		cannot_execute(client);
		return;
	}
	if (pid == 0) {
		char meth_env[255];
		char query_env[255];
		char length_env[255];

		dup2(cgi_output[1], 1);
		dup2(cgi_input[0], 0);
		close(cgi_output[0]);
		close(cgi_input[1]);
		sprintf(meth_env, "REQUEST_METHOD=%s", method);
		putenv(meth_env);
		if (strcasecmp(method, "GET") == 0) {
			sprintf(query_env, "QUERY_STRING=%s", query_string);
			putenv(query_env);
		}
		else {  
			sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
			putenv(length_env);
		}
		execl(path, path, NULL);
		exit(0);
	} else {   
		close(cgi_output[1]);
		close(cgi_input[0]);
		if (strcasecmp(method, "POST") == 0) {
			for (i = 0; i < content_length; i++) {
				recv(client, &c, 1, 0);
				write(cgi_input[1], &c, 1);
			}
		}
		
		while (read(cgi_output[0], &c, 1) > 0) {
			send(client, &c, 1, 0);
		}

		close(cgi_output[0]);
		close(cgi_input[1]);
		waitpid(pid, &status, 0);
	}
}

int get_line(int sock, char *buf, int size) {
	int i = 0;
	char c = '\0';
	int n;

	while ((i < size - 1) && (c != '\n')) {
		n = recv(sock, &c, 1, 0);
		if (n > 0) {
			if (c == '\r') {
				if ((n > 0) && (c == '\n')) {
					recv(sock, &c, 1, 0);
				} else {
					c = '\n';
				}
			}
			buf[i] = c;
			i++;
		} else {
			c = '\n';
		}
	}
	buf[i] = '\0';
	return(i);
}

void headers(int client, const char *filename) {
	char buf[1024];
	(void)filename; 

	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, SERVER_STRING);
	send(client, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(client, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(client, buf, strlen(buf), 0);
}


void serve_file(int client, const char *filename) {
	FILE *resource = NULL;
	int numchars = 1;
	char buf[1024];

	buf[0] = 'A'; buf[1] = '\0';
	while ((numchars > 0) && strcmp("\n", buf)) {
		numchars = get_line(client, buf, sizeof(buf));
	}

	resource = fopen(filename, "r");
	if (resource == NULL) {
		not_found(client);
	} else {
		headers(client, filename);
		cat(client, resource);
	}
	fclose(resource);
}

int startup(u_short *port) {
	int httpd = 0;
	struct sockaddr_in name;

	httpd = socket(PF_INET, SOCK_STREAM, 0);
	if (httpd == -1) {
		error_die("socket");
	}

	memset(&name, 0, sizeof(name));
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0) {
		error_die("bind");
	}
	if (*port == 0) {
		int namelen = sizeof(name);
		if (getsockname(httpd, (struct sockaddr *)&name,(socklen_t * ) &namelen) == -1) {
			error_die("getsockname");
		}
		*port = ntohs(name.sin_port);
	}
	if (listen(httpd, 128) < 0) {
		error_die("listen");
	}
	return(httpd);
}


void StopServer(){
	close(server_socket);
	_exit(0);
}

int main(void) {

	int timeoutsetres;
	u_short port = 1025;
	int client_sock = -1;
	struct sockaddr_in client_name;
	int client_name_len = sizeof(client_name);
	struct timeval timeout={3,0};
	pthread_t newthread;

	server_socket = startup(&port);
	printf("httpd running on port %d\n", port);
	signal(SIGINT, StopServer); 

	while (1) {

		client_sock = accept(server_socket, (struct sockaddr *)&client_name, (socklen_t *)&client_name_len);
		timeoutsetres = setsockopt(client_sock,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
		timeoutsetres = setsockopt(client_sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));

		if (client_sock == -1) {
			error_die("accept");
		}

		if (pthread_create(&newthread , NULL, (void *(*)(void *))accept_request, & client_sock) != 0) {
			perror("pthread_create");
		}
	}
	close(server_socket);
	return(0);
}
