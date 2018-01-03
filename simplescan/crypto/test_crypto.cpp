#include <iostream>
#include <iomanip>
#include <string>
#include <mutex>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include "test_handshake.h"

using namespace std;

void debug_print(const char* name, const Ring::ByteArr& arr)
{
    printf("%s (%zu bytes): ", name, arr.size());
    for (auto c: arr)
    {
        printf("%02x", c);
    }
    printf("\n");
}

int main()
{
    //  this code demonstrates how to use lib and prints all the internal data
    //  so the developer can compare and check bytes in output

    //  TODO move out debug print from library internals to users callback
    //  TODO in bright future this can be converted to unit tests

    //  use this to generate signing key pair that shoud be embedded into software
    //auto pp = Ring::GenSignPK();

    using Ring::ByteArr;

    //  we use *_rx and *_tx as dummy transport layer for handshake
    //  same applies to lambdas that are passed in constructors
    ByteArr client_rx;
    ByteArr client_tx;
    mutex rx_mutex;
    mutex tx_mutex;

    Ring::EncHandshake client(
        //  this is dummy client->server transport
        [&](const ByteArr& data)
        {
            lock_guard<mutex> lock(tx_mutex);
            client_tx = data;
        },
        //  this is dummy server->client transport
        [&]()
        {
            auto tmp = ByteArr{};
            while(tmp.size() == 0) {
                lock_guard<mutex> lock(rx_mutex);
                tmp = client_rx;
                this_thread::sleep_for(chrono::milliseconds(1));
            }
            return tmp;
        }
    );

    Ring::EncHandshake server(
        //  this is dummy server->client transport
        [&](const ByteArr& data)
        {
            lock_guard<mutex> lock(rx_mutex);
            client_rx = data;
        }
    );

    //  server operates in different thread (pretend this is different device)
    auto server_thread = thread(
        [&]()
        {
            //  waiting until client sends its public key
            auto tmp = ByteArr{};
            while(tmp.size() == 0) {
                lock_guard<mutex> lock(tx_mutex);
                tmp = client_tx;
                this_thread::sleep_for(chrono::milliseconds(1));
            }

            //  start handshake
            server.doHandshake(tmp);
        }
    );

    //  pretend we start handshake from smartphone
    client.doHandshake();

    //  wait until client and server do their shady job
    if (server_thread.joinable())
        server_thread.join();

    //  lets test secret shared key
    auto msg = ByteArr{0x01, 0x02, 0x03, 0x04, 0x5, 0x06, 0xa, 0xb, 0xc, 0xd, 0xf, 0x11, 0x22};
    debug_print("[main] msg", msg);

    auto encr = client.encrypt(msg);
    debug_print("[main] encrypted", encr);

    auto decr = server.decrypt(encr);
    debug_print("[main] decrypted", decr);

    //  this is to show how nonce is incremented internally
    for (int i = 0; i < 10; ++i)
        client.encrypt(msg);

    return 0;
}
