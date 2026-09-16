# ExamRank06 — `mini_serv`

A solution for the 42 Exam Rank 06 `mini_serv` exercise.

The program is a non-blocking TCP chat server. It listens only on
`127.0.0.1`, assigns each connected client an ID, and broadcasts messages to
all other connected clients.

## Requirements

- macOS or Linux
- A C compiler such as `cc` or `clang`
- `nc` (netcat) for testing

## Build

Compile the server with:

```sh
cc -Wall -Wextra -Werror miniserv.c -o mini_serv
```

## Run

Pass the listening port as the only argument:

```sh
./mini_serv 6667
```

In separate terminals, connect clients with:

```sh
nc 127.0.0.1 6667 or nc localhost 6667
```

Messages sent by one client are broadcast to every other connected client.
The server also announces when clients connect or disconnect.

Stop the server with `Ctrl-C`.

## Reference files

- `miniserv.c` — submitted server implementation
- `subject/subject.en.txt` — exercise requirements
- `subject/main.c` — provided starter code
- `subject/server.py` — reference/testing server

## Cleanup

Remove the compiled executable with:

```sh
rm -f mini_serv
```