ACP-C: Arcus C Client Performance benchmark program

Compile
-------

Modify Makefile according to your setup.  And run "make".  It should generate
the executable "acp".

Run
---

Modify prep.bash according to your setup.  acp needs zookeeper and Arcus C
client libraries.  prep.bash sets up LD_LIBRARY_PATH.

Then run acp using one of the config files.

    . ./prep.bash
    ./acp -config config-simple-decinc.txt

How it works
------------

The main thread creates one or more pools, and then creates
a number of worker threads.  Each worker thread executes the test code
specified in the config file (client_profile).

There is a primitive rate control.  Each worker thread sleeps a number of
milliseconds between commands to approximate the rate specified in the
config file.

How we use it
-------------

Mostly used to test Arcus 1.7's replication code.

License
-------

Licensed under the Apache License, Version 2.0: http://www.apache.org/licenses/LICENSE-2.0


