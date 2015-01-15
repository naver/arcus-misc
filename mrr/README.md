mrr - network latency checker

Compile
-------

To build `mrr` from the git-cloned source code, just do `make`.

```
$ make
```

Run
---

First, check the usage with fillowing command.

```
$ ./mrr -h
```

To start as a server process, run mrr with `-s` option like followings.

```
$ ./mrr -s -addr <IP> -port <port>
```

To start as a client process, run mrr withoud `-s` option like followings.

```
$ ./mrr -addr <server IP> -port <server port>
```

Then, the client shows the network latencies between the client and the server.

License
-------

Licensed under the Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0
