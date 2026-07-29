#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

int count = 0, max_fd = 0, sockfd = -1;
int ids[65536];
char *msgs[65536];
char *to_send[65536];

fd_set rfds, wfds, afds;
char buf_read[1001], buf_write[42];

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void fatal_error(void)
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

void notify_other(int author, char *str)
{
    char *tmp;
    for(int fd = 0; fd <= max_fd; fd++)
    {
        if(fd != author && fd != sockfd && FD_ISSET(fd, &afds))
        {
            tmp = str_join(to_send[fd], str);
            if(tmp == 0)
                fatal_error();
            to_send[fd] = tmp;
        }
    }
}
void register_client(int fd)
{
    max_fd = fd > max_fd ? fd : max_fd;
    ids[fd] = count++;
    msgs[fd] = 0;
    to_send[fd] = 0;
    FD_SET(fd, &afds);
    sprintf(buf_write, "server: client %d just arrived\n", ids[fd]);
    notify_other(fd, buf_write);
}

void remove_client(int fd)
{
    sprintf(buf_write, "server: client %d just left\n", ids[fd]);
    notify_other(fd, buf_write);
    free(msgs[fd]);
    free(to_send[fd]);
    msgs[fd] = 0;
    to_send[fd] = 0;
    FD_CLR(fd, &afds);
    close(fd);
}

void send_msg(int fd)
{
    char *msg;
    while(extract_message(&(msgs[fd]), &msg))
    {
        sprintf(buf_write, "client %d: ", ids[fd]);
        notify_other(fd, buf_write);
        notify_other(fd, msg);
        free(msg);
    }
}

void flush_pending(void)
{
    int fd, len, sent;
    char *tmp;

    for(fd = 0; fd <= max_fd; fd++)
    {
        if(FD_ISSET(fd, &wfds) && to_send[fd] != NULL)
        {
            len = strlen(to_send[fd]);
            sent = send(fd, to_send[fd], len, MSG_NOSIGNAL);
            if(sent == len)
            {
                free(to_send[fd]);
                to_send[fd] = 0;
            }
            else if(sent > 0)
            {
                tmp = str_join(0, to_send[fd] + sent);
                if(tmp == 0)
                    fatal_error();
                free(to_send[fd]);
                to_send[fd] = tmp;
            }
        }
    }

}

int create_socket()
{
    max_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(max_fd < 0)
        fatal_error();
    FD_SET(max_fd, &afds);
    return max_fd;
}

int main(int ac, char **av)
{
    if(ac != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }

    FD_ZERO(&afds);
    sockfd = create_socket();

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433);
    servaddr.sin_port = htons(atoi(av[1]));

    if(bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))
        fatal_error();
    if(listen(sockfd, SOMAXCONN))
        fatal_error();

    while(1)
    {
        rfds = afds;
        FD_ZERO(&wfds);

        for(int fd = 0; fd <= max_fd; fd++)
        {
            if(to_send[fd] != NULL)
                FD_SET(fd, &wfds);
        }
        if(select(max_fd + 1, &rfds, &wfds, NULL, NULL) < 0)
            fatal_error();
        flush_pending();
        for(int fd = 0; fd <= max_fd; fd++)
        {
            if(!FD_ISSET(fd, &rfds))
                continue;
            if(fd == sockfd)
            {
                struct sockaddr_in cli;
                socklen_t addr_len = sizeof(cli);
                int client_fd = accept(sockfd, (struct sockaddr *)&cli, &addr_len);
                if(client_fd >= 0)
                    register_client(client_fd);
            }
            else
            {
                int read_bytes = recv(fd, buf_read, 1000, 0);
                if(read_bytes <= 0)
                    remove_client(fd);
                else
                {
                    buf_read[read_bytes] = '\0';
                    char *tmp = str_join(msgs[fd], buf_read);
                    if(tmp == 0)
                        fatal_error();
                    msgs[fd] = tmp;
                    send_msg(fd);
                }
            }
        }
    }
}