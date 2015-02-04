mrr - network latency checker

Compile
-------

To build `mrr` from the git-cloned source code, just do `make`.

```
$ make
```

Run
---

First, check the usage with `-h` opttion.

```
$ ./mrr -h
mrr <options>
-h             print mrr usage
-s             server mode
-addr ip       server address
-port port     server port
-backlog num   listen backlog
-sleep usec    sleep <usec> microseconds between requests
-m size        message size in bytes
-t threads     request-response threads
```

To start as a server process, run mrr with `-s` option like followings.

```
$ ./mrr -s -addr <IP> -port <port>
```

To start as a client process, run mrr without `-s` option like followings.

```
$ ./mrr -addr <server IP> -port <server port>
```

Then, the client shows the network latencies between the client and the server.

License
-------

Licensed under the Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0
