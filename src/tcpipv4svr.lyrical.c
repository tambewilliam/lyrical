
// ---------------------------------------------------------------------
// Copyright (c) William Fonkou Tambe
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ---------------------------------------------------------------------


// File which implement the TCPIPv4 server.

#define HTTPRQSTBUFSZ (2*1024*1024)
// Buffer to hold an HTTP request.
static u8* httprqstbuf;

globalvarregion[3] = (httprqstbuf = mmalloc(HTTPRQSTBUFSZ));

string filedirnamearg0 = filedirname(arg[0]);

if (chdir(filedirnamearg0.ptr) == -1) {
	perror("lyrical: chdir()");
	exit(-1); // The shell use non-null for failing.
}

mmfree(filedirnamearg0.ptr);

int socketfd = socket(AF_INET, SOCK_STREAM, 0);

if (socketfd == -1) {
	perror("lyrical: socket()");
	exit(-1); // The shell use non-null for failing.
}

if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1) {
	perror("lyrical: setsockopt(SO_REUSEADDR)");
	exit(-1); // The shell use non-null for failing.
}

if (bind(
	socketfd,
	(struct sockaddr*)&options.tcpipv4svr.addr,
	sizeof(options.tcpipv4svr.addr)) == -1) {
	perror("lyrical: bind()");
	exit(-1); // The shell use non-null for failing.
}

#define MAXPENDINGCONNECTIONS 64
if (listen(socketfd, MAXPENDINGCONNECTIONS) == -1) {
	perror("lyrical: listen()");
	exit(-1); // The shell use non-null for failing.
}

fprintf(stderr, "lyrical: ready to accept connections\n");

struct sockaddr_in clientaddr;

while (1) {
	
	socklen_t clientaddrlen = sizeof(clientaddr);
	
	int acceptfd;
	while ((acceptfd = accept(socketfd, (struct sockaddr*)&clientaddr, &clientaddrlen)) == -1)
		perror("lyrical: accept()");
	
	#define SCKFD 3
	if (dup2(acceptfd, SCKFD) == -1) {
		perror("lyrical: dup2()");
		exit(-1); // The shell use non-null for failing.
	} else close(acceptfd);
	
	// Read the HTTP request.
	// Note that I read at most (HTTPRQSTBUFSZ-1)
	// so to have the last byte available
	// for use by the null-terminating byte.
	ssize_t n;
	n = read(SCKFD, httprqstbuf, HTTPRQSTBUFSZ-1);
	
	if (n == -1) {  /* read failure stop now */
		perror("lyrical: read()");
		exit(-1); // The shell use non-null for failing.
	} else if (n == (HTTPRQSTBUFSZ-1)) {
		fprintf(stderr, "request header >= %u which is the max\n", (HTTPRQSTBUFSZ-1));
		exit(-1); // The shell use non-null for failing.
	}
	
	// Null-terminate the data read.
	httprqstbuf[n] = 0;
	
	// ### Enable to dump HTTP request.
	// ### Turn this into a compiler option.
	#if 0
	fprintf(stderr, "HTTP request dump ----\n");
	write(2, httprqstbuf, n);
	fprintf(stderr, "----------------------\n");
	#endif
	
	pid_t pid = fork();
	
	if (pid == -1) {
		perror("lyrical: fork()");
		exit(-1); // The shell use non-null for failing.
	} else if (pid) {
		// Parent process.
		//close(SCKFD); // Using close() prevent re-use of the file-descriptor.
	} else {
		// Child process.
		
		close(socketfd);
		
		// This function return 1 if
		// the request is not a GET
		// HTTP request for an exiting
		// supported file.
		uint isrespdynamic () {
			// If not a GET HTTP request,
			// assume a dynamic response.
			if (!stringiseqnocase4("GET ", httprqstbuf, 4))
				return 1;
			
			// Parse the request url.
			u8* urlstart = httprqstbuf;
			while (*++urlstart != ' ');
			while (*++urlstart == ' ');
			u8* urlend = urlstart;
			u8 urlendchar;
			while ((urlendchar = *++urlend) != ' ' &&
				urlendchar != '\r' &&
				urlendchar != '\n');
			
			u8* urldecoded;
			uint urldecodedlen;
			
			// Function used to decode the request url.
			void urldecode () {
				// +2 account for "./" which will be
				// prefixed to the decoded request url.
				urldecoded = mmalloc(((uint)urlend - (uint)urlstart) +2);
				
				u8* d = urldecoded;
				
				// Prefix the decoded request url with "./" .
				(*d++) = '.';
				(*d++) = '/';
				
				u8* s = urlstart;
				
				while (s < urlend) {
					
					u8 c = *s++;
					
					if (c == '%' && (s+1) < urlend) {
						
						u8 c2 = *s++;
						u8 c3 = *s++;
						
						uint ishexdigit (u8 c) {
							return ((c >= '0' && c <= '9') ||
								(c >= 'a' && c <= 'f') ||
								(c >= 'A' && c <= 'F'));
						}
						
						u8 tolower (u8 c) {
							if (c >= 'A' && c <= 'F')
								return (c+('a'-'A'));
							else return c;
						}
						
						if (ishexdigit(c2) && ishexdigit(c3)) {
							
							c2 = tolower(c2);
							c3 = tolower(c3);
							
							if (c2 <= '9')
								c2 -= '0';
							else c2 = (c2 - 'a') + 10;
							
							if (c3 <= '9')
								c3 -= '0';
							else c3 = (c3 - 'a') + 10;
							
							(*d++) = ((c2 * 16) + c3);
							
						} else {
							
							(*d++) = c;
							(*d++) = c2;
							(*d++) = c3;
						}
						
					} else if (c == '+') (*d++) = ' ';
					else (*d++) = c;
				}
				
				*d = 0;
				
				urldecodedlen = (uint)d - (uint)urldecoded;
			}
			
			urldecode();
			
			// Parent directory access is illegal.
			if (stringsearchright7(urldecoded, urldecodedlen, STRWLEN("/../"))) {
				mmfree(urldecoded);
				return 1;
			}
			
			struct {
				u8* ext;
				uint extlen;
				u8* type;
			} mimes[] = {
			{STRWLEN(".html"), "text/html" },
			{STRWLEN(".htm"),  "text/html" },
			{STRWLEN(".gif"),  "image/gif" },
			{STRWLEN(".jpg"),  "image/jpg" },
			{STRWLEN(".jpeg"), "image/jpeg"},
			{STRWLEN(".png"),  "image/png" },
			{STRWLEN(".ico"),  "image/ico" },
			{STRWLEN(".zip"),  "image/zip" },
			{STRWLEN(".gz"),   "image/gz"  },
			{STRWLEN(".tar"),  "image/tar" },
			{0, 0, 0}};
			
			// Check whether the file requested is supported.
			
			uint mimeidx = 0;
			
			while (1) {
				
				u8* mimeext = mimes[mimeidx].ext;
				
				if (mimeext) {
					
					uint mimeextlen = mimes[mimeidx].extlen;
					
					if (stringiseqnocase4(urldecoded+(urldecodedlen-mimeextlen), mimeext, mimeextlen))
						break;
					
				} else {
					mmfree(urldecoded);
					return 1;
				}
				
				++mimeidx;
			}
			
			int fd = open(urldecoded, O_RDONLY);
			
			mmfree(urldecoded);
			
			if (fd == -1) return 1;
			
			// lseek() to the file end to find its size.
			off_t filesz = lseek(fd, 0, SEEK_END);
			
			// lseek() back to the file start to read it.
			lseek(fd, 0, SEEK_SET);
			
			dprintf(SCKFD,
				"HTTP/1.1 200 OK\nContent-Length: %lu\nConnection: close\nContent-Type: %s\n\n",
				filesz, mimes[mimeidx].type);
			
			// Not all platforms support sendfile().
			#if defined(__linux__)
			if (sendfile(SCKFD, fd, 0, filesz) == -1) {
				perror("lyrical: sendfile()");
				exit(-1); // The shell use non-null for failing.
			}
			#else
			ssize_t n;
			while ((n = read(fd, httprqstbuf, HTTPRQSTBUFSZ)) > 0)
				write(SCKFD, httprqstbuf, n);
			#endif
			
			return 0;
		}
		
		if (isrespdynamic()) runexecpages();
		
		fsync(SCKFD);
		shutdown(SCKFD, SHUT_WR); // Note that shutdown(SHUT_WR) is used so that FIN packet get sent.
		exit(0);
	}
}
