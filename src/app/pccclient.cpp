#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <iostream>
#include "../core/udt.h"
#include "../core/options.h"
#include <signal.h>

#define DATA_BATCH_SIZE 1000000

using namespace std;

struct monitor_args {
    UDTSOCKET* client_ptr;
    const char* output_filename;
};

void* monitor(void*);

UDTSOCKET client;
bool stop = false;

void intHandler(int dummy) {
    stop = true;
}

void prusage() {
    cout << "usage: .../pccclient <send|recv> server_ip server_port outputfile [OPTIONS]" << endl;
    exit(-1);
}

int main(int argc, char* argv[]) {

    if ((argc < 5) || (0 == atoi(argv[3]))) {
        prusage();
    }
    const char* send_str = argv[1];
    const char* ip_str   = argv[2];
    const char* port_str = argv[3];
    const char* output_filename = argv[4];

    struct monitor_args m_args;
    m_args.output_filename = output_filename;
    Options::Parse(argc, argv);

    bool should_send = !strcmp(send_str, "send");

	signal(SIGINT, intHandler);

    // use this function to initialize the UDT library
    UDT::startup();

    struct addrinfo hints, *local, *peer;

    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (0 != getaddrinfo(NULL, "9000", &hints, &local)) {
        cout << "invalid network address.\n" << endl;
        return 0;
    }

    client = UDT::socket(local->ai_family, local->ai_socktype, local->ai_protocol);
    m_args.client_ptr = &client;

    freeaddrinfo(local);

    if (0 != getaddrinfo(ip_str, port_str, &hints, &peer)) {
        cout << "incorrect server/peer address. " << ip_str << ":" << port_str << endl;
        return 0;
    }

    // connect to the server, implict bind
    if (UDT::ERROR == UDT::connect(client, peer->ai_addr, peer->ai_addrlen)) {
        cout << "connect: " << UDT::getlasterror().getErrorMessage() << endl;
        return 0;
    }
    freeaddrinfo(peer);

    // pthread_create(new pthread_t, NULL, monitor, &client);
    pthread_create(new pthread_t, NULL, monitor, &m_args);


    int batch_size = DATA_BATCH_SIZE;
    char* data = new char[batch_size];
    bzero(data, batch_size);

    while (!stop) {
        int cur_size = 0;
        int this_call_size = 0;
        while (!stop && cur_size < batch_size) {

            if (should_send) {
                this_call_size = UDT::send(client, data + cur_size, batch_size - cur_size, 0);
            } else {
                this_call_size = UDT::recv(client, data + cur_size, batch_size - cur_size, 0);
            }

            if (this_call_size == UDT::ERROR) {
                cout << "send/recv: " << UDT::getlasterror().getErrorMessage() << std::endl;
                break;
            }

            cur_size += this_call_size;
        }
    }

    UDT::close(client);

    delete [] data;

    // use this function to release the UDT library
    UDT::cleanup();

    return 0;
}

void* monitor(void* s) {
    struct monitor_args* args = (struct monitor_args*)s;

    UDTSOCKET u = *(UDTSOCKET*)args->client_ptr;
    UDT::TRACEINFO perf;

    ofstream output_file(args->output_filename);
    if (output_file.is_open()) {
        output_file << "Index\tRate(Mbps)\tRTT(ms)\tSent\tLost" << endl;
        int i = 0;
        while (true) {
            sleep(1);
            i++;
            if (UDT::ERROR == UDT::perfmon(u, &perf)) {
                cout << "perfmon: " << UDT::getlasterror().getErrorMessage() << endl;
                break;
            }
            output_file   << i << "\t"
                          << perf.mbpsSendRate    << "\t\t"
                          << perf.msRTT           << "\t\t"
                          << perf.pktSentTotal    << "\t\t"
                          << perf.pktSndLossTotal << endl;
        }
        output_file.close();
    }
    return NULL;
}
