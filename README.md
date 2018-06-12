Privacy-Preserving Cloud, Email and Password Systems
==========

This is the central repository for the our CS Capstone project.

Members:

* Andrew Ekstedt
* Scott Merrill
* Scott Russell

Building
-----

Building is as simple as running

    ./make.sh

This will build all the dependencies of s2a (in the `third_party` directory)
and then build s2a.

If you have already run `make.sh`, you can build just s2a by running `make`
in the `src` directory.

    cd src && make

Usage
----

### Command-line

The program is split into two halves: a client and a server.

#### Server

To run the server, simply run

    ./server

It will sit in the foreground and listen for requests from the client.
By default, the server listens on port 24992.
To listen on another port, pass the port number as the first argument to the server.

    ./server 8000

#### Client

The client is a little more interesting.
It has a number of subcommands;
to see the full list, give it the `-h` option (or any other invalid option

    % ./client -h
    ./client: invalid option -- 'h'
    Usage: client [-p port] <command> [args]
    Commands:
        client setup [files...]
        client search <word>
        client add <fileid> [words...]
        client addfile <filename>
        client delete <fileid> [words...]

For any of these commands to work, the server must already be running.
If the server is not running, the client will hang until the server is started.

By default, the client tries to connect to port 24992.
To tell the client to connect to a different port, use the lowercase `-p` option.

    ./client -p 8000 search test

#### Setup

The setup command creates an encrypted search index

    client setup [files...]

For example,

    ./client setup *.cpp

will initialize the search index with all cpp files in the current directory.

There is also a hidden `setuplist` command, which reads the list of files from another file
instead of the command line.

For example,

    ls *.cpp >filelist
    ./client setuplist filelist

This does the same thing as the previous setup command, but can be used even if
the number of files is too large to fit on the command line.

The uppercase `-P` option  controls which variant of the algorithm is used.
Pass it once to use the packed variant. Pass it twice to use the pointer variant.

    ./client setup -P *.cpp         # packed
    ./client setup -PP *.cpp        # pointer

#### Search

The search command searches for a keyword in the encrypted index.
It prints out a list of matching files.

    client search [keyword]

For example,

    % ./client search int
    info: loaded client state
    got response
    int: bench.cpp
    int: client.cpp
    int: client_test.cpp
    int: DSSEClient.cpp
    int: DSSE.cpp
    int: DSSEServer.cpp
    int: fail.cpp
    int: merged-client.cpp
    int: server.cpp
    int: speed.cpp
    int: storage.cpp

#### Add

The add command adds one or more keywords to a file.
To use the add command, you need to know the id of the file you want to modify.

    client add <fileid> [keywords]

For example,

    ./client add 1 hello

Now searching for 'hello' will return the file with id number 1.

    % ./client search hello
    info: loaded client state
    got response
    hello: bench.cpp

There is also a variant of the add command called addfile.
The addfile command adds an entire new file to the search index.

    % ./client addfile DSSE.h

#### Delete

The delete command deletes one or more keywords from a file.
It works similarly to the add command.
To use the delete command, you need to know the id of the file you want to modify.

    client delete <fileid> [keywords]

For example,

    ./client delete 1 hello

Hacking
-----

The top level of the repository is layed out as follows.

    Benchmarks/           benchmark results
    Meeting_Notes/        meeting notes throughout the term
    Poster/               our expo poster
    Term_PowerPoints/     powerpoint slides from our progress reports
    demo/                 (demo_app branch only) this is where the demo app lives
    reports/              capstone reports and other documents
    src/                  the main source code
    third_party/          copies of third-party libraries that we depend on

The most interesting directory is `src`, which contains all the main code for the DSSE.

    DSSE.h                        central header file
    DSSE.cpp                      core DSSE class (all cryptographic code)
    DSSEClient.cpp                DSSE client class  (all client networking logic)
    DSSEServer.cpp                DSSE server class  (all server networking logic)
    DSSETokenize.cpp              tokenization code
    storage.cpp                   storage serialization code
    
    dsse-prototype.py             a prototype written in Python
    dsse.proto                    protobuf definitions
    
    bench.cpp                     some benchmarking code
    client.cpp                    client program - uses DSSE::Client
    server.cpp                    server program - uses DSSE::Server


The DSSE is organized around three central classes ---
`DSSE::Core`, `DSSE::Client`, and `DSSE::Server`.
There is one central header file, `DSSE.h`,
which contains declarations for all these classes and their associated methods.

`DSSE.cpp` defines the `DSSE::Core` class, and
contains all the low-level cryptographic code.
All the algorithms described in the paper are implemented in this file.
It contains both the client and server halves of the Setup, Search, Add, and Delete methods.
It does not contain any networking code.
The Core class is also where all the state required by the algorithms are stored.
`storage.cpp` contains functions which are able to serialize the this state into files.

`DSSEClient.cpp` and `DSSEServer.cpp` define the
`DSSE::Client` and `DSSE::Server` classes,
which implement the client and server halves of the network communication, respectively.
All the networking is performed in these files.
These classes interact with the `DSSE::Core` class;
they deserialized messages from the wire, pass them to the core, and serialize the results back out.
Messages are serialized using protobuf; messages definitions can be found in `dsse.proto`.

`dsse-prototype.py` contains a prototype implementation of the core DSSE protocol (basic variant).
If you just want to understand the algorithm, this may be a good place to look.

Finally, `client.cpp` and `server.cpp` are the
actual client and server programs. This is where the `main` function is
defined.
For the most part, these are just simple wrappers around the
`DSSE::Client` and `DSSE::Server` classes;
they parse command line arguments, construct an object of the appropriate class,
call one or more methods, and print the results.
`bench.cpp` is the program we used for some of the benchmarks.

Further details and documentation of the classes can be found in `DSSE.h`.
