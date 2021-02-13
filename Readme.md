# PCC

This repository houses the Performance Congestion Control project.

To build PCC, run the following:

```bash
cd src
make
```

This will produce two apps (pccclient and pccserver) in the src/app directory.

To test that PCC is functioning, you can run:

```bash
cd src
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/core/
./app/pccserver recv 9000
```

(this creates a PCC server that listens on port 9000 and receives data)

and in a separate terminal, run:

```bash
cd src
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/core/
./app/pccclient send 127.0.0.1 9000
```

(this create a PCC client that connects to the local host (IP 127.0.0.1) at
port 9000, then sends data to the server at that address.
