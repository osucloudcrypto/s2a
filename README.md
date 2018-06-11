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

#### Setup

The setup command creates an encrypted search index

    client setup [files...]

For example,

    ./client setup *.cpp

will initialize the search index with all cpp files in the current directory.
If

There is also a hidden `setuplist` command, which reads the list of files from another file
instead of the command line.

For example,

    ls *.cpp >filelist
    ./client setuplist filelist

This does the same thing as the previous setup command, but can be used even if
the number of files is too large to fit on the command line.

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

Now searching for `hello' will return the file with id number 1.

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
