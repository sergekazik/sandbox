#include <iostream>
#include <iomanip>
#include <string>
#include <mutex>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sodium.h>

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define TEST_SODIUM_DIRECT  0
#define KEY_SIZE            32

#include "version.h"
#include "sodium_glue.h"
#include "RingCrypto.hh"

#define IO_MSG_BUFFER_SIZE 2000
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

#if !TEST_SODIUM_DIRECT
static bool print_help(char* arg)
{
    if (!strcmp(arg, "--help") || !strcmp(arg, "-h") || !strcmp(arg, "?"))
    {
        printf("test_crypto v. %s\nusage:\n--------------------------------------\n",version_date);
        printf("-gen            generate pub/sec keys pair\n");
        printf("-gpk            get public key\n");
        printf("-ppp <payload>  process public payload\n");
        printf("-enc <input>    encrypt input\n");
        printf("-dec <input>    decrypt input\n");
        printf("--------------------------------------\nExtra:\n");
        printf("-gpw            -gpk, -ppp, -enc\n");
        printf("-ppf            -ppp from file\n");
        printf("-def            -dec from file\n");

        return true;
    }
    return false;
}

static bool print_gen(char *arg)
{
    if (!strcmp(arg, "--generate") || !strcmp(arg, "-gen"))
    {
        (void) sodium_init();
        unsigned char pk[KEY_SIZE];
        unsigned char sk[KEY_SIZE];

        crypto_box_seed_keypair(pk, sk, (const unsigned char*) "3245;lkjh fd[0g946 /ljf"); // just string to seed
        crypto_sign_keypair(pk, sk);

        Ring::ByteArr tpk(pk, pk + KEY_SIZE);
        Ring::ByteArr tsk(sk, sk + KEY_SIZE);
        debug_print("pub", tpk);
        debug_print("prv", tsk);
        return true;
    }
    return false;
}

int main(int argc, char* argv[])
{
    bool bServerCmdline = false;
    char outstr[IO_MSG_BUFFER_SIZE];
    if (argc > 1)
    {
        if (print_help(argv[1]))
            return Ring::Ble::Crypto::Error::NO_ERROR;
        if (print_gen(argv[1]))
            return Ring::Ble::Crypto::Error::NO_ERROR;

        // suppress all debug if in the command mode
        Ring::Ble::Crypto::Debug::Suppress(true);
    }

    char client_public[IO_MSG_BUFFER_SIZE];
    int client_public_lenght = sizeof(client_public);

    char server_public[IO_MSG_BUFFER_SIZE];
    int server_public_lenght = sizeof(server_public);

    char crypted[IO_MSG_BUFFER_SIZE];
    int crypted_len = sizeof(crypted);

    char decrypted[IO_MSG_BUFFER_SIZE];
    int decrypted_len = sizeof(decrypted);

    char msg[IO_MSG_BUFFER_SIZE] = "HelloServerMessage-fromClient";

    memset(client_public, 0, IO_MSG_BUFFER_SIZE);
    memset(server_public, 0, IO_MSG_BUFFER_SIZE);

    if (argc >= 2 && !strcmp(argv[1], "-server"))
    {
        // argv[1], "-server"
        // argv[2], "gpk" key from client no spaces
        // argv[3], "payload to decode" no spaces
        bServerCmdline = true;

        cout << "enter client PUBLIC_KEY>";
        cin >> outstr;
        client_public_lenght = strlen(outstr) / 2;
        for (int idx = 0; idx < client_public_lenght; idx++)
        {
            int val;
            sscanf(&outstr[idx*2], "%02x", &val);
            client_public[idx] = (unsigned char) val;
        }

        Ring::Ble::Crypto::Server server(client_public, client_public_lenght);
        server.GetPublicPayload(server_public, server_public_lenght);

        if (argc == 2) // asking for PublicPayload only
        {
            int offset = sprintf(outstr, "gpp:");
            for (int i = 0; i < server_public_lenght; i++)
                offset += sprintf(&outstr[offset], "%s%d", i?" ":"", (unsigned char) server_public[i]);
            sprintf(&outstr[offset], ":gpp");
            printf("%s\n", outstr);

            printf("DEC>");
            cin >> outstr;

            crypted_len = strlen(outstr) / 2;
            for (int idx = 0; idx < crypted_len; idx++)
            {
                int val;
                sscanf(&outstr[idx*2], "%02x", &val);
                crypted[idx] = (unsigned char) val;
                printf("%02X", val);
            }
            printf("\n");

            decrypted_len = sizeof(decrypted);
            memset(decrypted, 0, decrypted_len);
            server.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
            decrypted[decrypted_len]='\n';
            printf("TEST \"%s\" = %s (%d)\n", outstr, decrypted, decrypted_len);

            return Ring::Ble::Crypto::Error::NO_ERROR;
        }

    }

    Ring::Ble::Crypto::Client client;

    if (argc < 2 || (argc > 1 && (!strcmp(argv[1], "-gpk")||!strcmp(argv[1], "-gpw"))))
        client.GetPublicKey(client_public, client_public_lenght);

    // single command mode client get public key
    if (argc > 1 && (!strcmp(argv[1], "-gpk")||!strcmp(argv[1], "-gpw")))
    {
        memset(outstr, 0, IO_MSG_BUFFER_SIZE);

        int offset = sprintf(outstr, "HEX|");
        for (int i = 0; i < client_public_lenght; i++)
            offset += sprintf(&outstr[offset], "%s%02X", i?" ":"", (unsigned char) client_public[i]);
        sprintf(&outstr[offset], "|HEX");
        printf("%s\n", outstr);

        offset = sprintf(outstr, "KEY|");
        for (int i = 0; i < client_public_lenght; i++)
            offset += sprintf(&outstr[offset], "%02X", (unsigned char) client_public[i]);
        sprintf(&outstr[offset], "|KEY");
        printf("%s\n", outstr);

        offset = sprintf(outstr, "gpk:");
        for (int i = 0; i < client_public_lenght; i++)
            offset += sprintf(&outstr[offset], "%s%d", i?" ":"", (unsigned char) client_public[i]);
        sprintf(&outstr[offset], ":gpk");
        printf("%s\n", outstr);

        if (!strcmp(argv[1], "-gpw"))
        {
            printf("PPP>");
            cin >> outstr;

            server_public_lenght = strlen(outstr) / 2;
            for (int idx = 0; idx < server_public_lenght; idx++)
            {
                int val;
                sscanf(&outstr[idx*2], "%02x", &val);
                server_public[idx] = (unsigned char) val;
            }
            client.ProcessPublicPayload(server_public, server_public_lenght);
            client.Encrypt("helloz", 6, crypted, crypted_len);

            memset(outstr, 0, IO_MSG_BUFFER_SIZE);

            offset = sprintf(outstr, "HEX|");
            for (int i = 0; i < crypted_len; i++)
                offset += sprintf(&outstr[offset], "%s%02X", i?" ":"", (unsigned char) crypted[i]);
            sprintf(&outstr[offset], "|HEX");
            printf("%s\n", outstr);
            offset = sprintf(outstr, "DEC|");
            for (int i = 0; i < crypted_len; i++)
                offset += sprintf(&outstr[offset], "%02X", (unsigned char) crypted[i]);
            sprintf(&outstr[offset], "|DEC");
            printf("%s\n", outstr);

            offset = sprintf(outstr, "enc:");
            for (int i = 0; i < crypted_len; i++)
                offset += sprintf(&outstr[offset], "%s%d", i?" ":"", (unsigned char) crypted[i]);
            sprintf(&outstr[offset], ":enc");
            printf("%s\n--------------------------------\nselftest\n", outstr);

            // self test
            decrypted_len = sizeof(decrypted);
            client.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
            decrypted[decrypted_len]='\n';
            printf("%s\n", decrypted);
        }
        // client.SaveSecrets();
        return Ring::Ble::Crypto::Error::NO_ERROR;
    }

    // the prepared client_public key is supposed to be sent to server and used to init server
    Ring::Ble::Crypto::Server server(client_public, client_public_lenght);
    server.GetPublicPayload(server_public, server_public_lenght);
    // the prepared server_public payload to be sent back to client and processed

    if (bServerCmdline)
    {
        if (argc == 4) // asking to decode
        {
            char *in = argv[3];
            crypted_len = strlen(in) / 2;
            for (int idx = 0; idx < crypted_len; idx++)
            {
                int val;
                sscanf(&in[idx*2], "%02x", &val);
                crypted[idx] = (unsigned char) val;
            }
            server.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
            printf("dec:%s:dec\n", decrypted);
            return Ring::Ble::Crypto::Error::NO_ERROR;
        }
    }

    //-------------------------- step 2 -------------------------------------------------

    // single command mode client process public payload
    char out[1024];
    memset(out, 0, sizeof(out));
    if (argc > 2 && (!strcmp(argv[1], "-ppf") || !strcmp(argv[1], "-def")))
    {
        char *filename = argv[2]; // public_payload.log
        FILE *fin = fopen(filename, "rt");
        if (!fin)
        {
            printf("can't open payload file \"%s\". Abort.\n", filename);
            return -666;
        }

        char *ch, strline[255];
        int out_offset = 0;

        while (fgets(strline, 255, fin))
        {
            if (strstr(strline, "Flags") || strstr(strline, "Notifying"))
            {
                break;
            }

            printf("%s", strline);
            if (NULL != (ch = strchr(strline, ' ')) )
            {
                char *esc = NULL;
                esc = strrchr(strline, 27);
                ch = (esc?esc:ch)+1;
                if (*ch == ' ') ch++;

                int b[16];
                if (16 == sscanf(ch, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ",
                                 &b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7],&b[8],&b[9],
                                 &b[10],&b[11],&b[12],&b[13],&b[14],&b[15]))
                {
                    out_offset += sprintf(&out[out_offset],"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                                        b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],
                                        b[10],b[11],b[12],b[13],b[14],b[15]);
                }
                else
                {
                    char * ending = strstr(ch, "   ");
                    if (ending) *ending = '\0';

                    int num = sscanf(ch, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x ",
                                     &b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7],&b[8],&b[9],
                                     &b[10],&b[11],&b[12],&b[13],&b[14],&b[15]);

                    for (int i = 0; i < num; i++)
                    {
                        out_offset += sprintf(&out[out_offset], "%02x", b[i]);
                    }
                }
            }
        }
        fclose(fin);
    }

    if (argc > 2 && (!strcmp(argv[1], "-ppp") || !strcmp(argv[1], "-ppf")))
    {
        int val;
        char *in = strlen(out) ? out : argv[2];
        server_public_lenght = strlen(in) / 2;
        if (1 != sscanf(&in[0], "%02x", &val))
        {
            printf("\"%s\" - doesn't look like HEX payload\n", in);
            return -777;
        }
        else for (int idx = 0; idx < server_public_lenght; idx++)
        {
            sscanf(&in[idx*2], "%02x", &val);
            server_public[idx] = (unsigned char) val;
        }
    }

    int ret = client.ProcessPublicPayload(server_public, server_public_lenght);
    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.ProcessPublicPayload != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }

    if (argc > 2 && (!strcmp(argv[1], "-ppp") || !strcmp(argv[1], "-ppf")))
    {
        memset(outstr, 0, IO_MSG_BUFFER_SIZE);
        int offset = sprintf(outstr, "ppp:");
        for (int i = 0; i < server_public_lenght; i++)
            offset += sprintf(&outstr[offset], "%02x", (unsigned char) server_public[i]);
        sprintf(&outstr[offset], ":ppp");
        printf("%s\n", outstr);

        // client.SaveSecrets();
        return Ring::Ble::Crypto::Error::NO_ERROR;
    }

    //-------------------------- step 3 -------------------------------------------------
    // single command to encrypt the given message
    if (argc > 2 && !strcmp(argv[1], "-enc"))
    {
        sprintf(msg, "%s", argv[2]);
        // client.RestoreSecrets();
    }

    ret = client.Encrypt(msg, strlen(msg), crypted, crypted_len);
    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.Encrypt != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }

    if (argc > 2 && !strcmp(argv[1], "-enc"))
    {
        printf("msg:%s\n", msg);
        memset(outstr, 0, IO_MSG_BUFFER_SIZE);

        int offset = sprintf(outstr, "HEX|");
        for (int i = 0; i < crypted_len; i++)
            offset += sprintf(&outstr[offset], "%s%02X", i?" ":"", (unsigned char) crypted[i]);
        sprintf(&outstr[offset], "|HEX");
        printf("%s\n", outstr);
        offset = sprintf(outstr, "DEC|");
        for (int i = 0; i < crypted_len; i++)
            offset += sprintf(&outstr[offset], "%02X", (unsigned char) crypted[i]);
        sprintf(&outstr[offset], "|DEC");
        printf("%s\n", outstr);

        offset = sprintf(outstr, "enc:");
        for (int i = 0; i < crypted_len; i++)
            offset += sprintf(&outstr[offset], "%s%d", i?" ":"", (unsigned char) crypted[i]);
        sprintf(&outstr[offset], ":enc");
        printf("%s\n", outstr);
        return Ring::Ble::Crypto::Error::NO_ERROR;
    }

    //-------------------------- step 4 -------------------------------------------------
    // single command to decrypt the given message
    if (argc > 2 && (!strcmp(argv[1], "-dec") || !strcmp(argv[1], "-def")))
    {
        char *in = strlen(out) ? out : argv[2];
        crypted_len = strlen(in) / 2;
        for (int idx = 0; idx < crypted_len; idx++)
        {
            int val;
            sscanf(&in[idx*2], "%02x", &val);
            crypted[idx] = (unsigned char) val;
        }
        // client.RestoreSecrets();
        ret = client.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
    }
    else
    {
        // crypted is to be sent to server and decoded by it
        ret = server.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
        printf("\n\n----------------------\nServer decpypted %d bytes:\n%s\n-------------------------------\n\n", decrypted_len, decrypted);
    }

    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.Decrypt != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }

    decrypted[decrypted_len]='\0';

    if (argc > 2 && (!strcmp(argv[1], "-dec") || !strcmp(argv[1], "-def")))
    {
        printf("enc:%s:enc\n", argv[2]);
        printf("dec:%s:dec\n", decrypted);
        return Ring::Ble::Crypto::Error::NO_ERROR;
    }

    printf("test ok: origin: %s\n", msg);
    Ring::Ble::Crypto::Debug::DumpArray("encrypted:", Ring::Ble::Crypto::ByteArr(crypted, crypted + crypted_len));
    printf("test ok: decrypted: %s\n", decrypted);

    // here is test of backward functionality when server encrypt and client decrypt it

    sprintf(msg, "HelloClientMessage-fromServer");
    ret = server.Encrypt(msg, strlen(msg), crypted, crypted_len);
    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.Encrypt != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }

    // crypted is to be read by client and decoded
    ret = client.Decrypt(crypted, crypted_len, decrypted, decrypted_len);
    if (ret != Ring::Ble::Crypto::Error::NO_ERROR)
    {
        // some error, abort
        printf("ret.Decrypt != Crypto::Error::NO_ERROR) = %d\n", ret);
        return ret;
    }
    printf("test ok: origin: %s\n", msg);
    Ring::Ble::Crypto::Debug::DumpArray("encrypted:", Ring::Ble::Crypto::ByteArr(crypted, crypted + crypted_len));
    decrypted[decrypted_len]='\0';
    printf("test ok: decrypted: %s\n", decrypted);

    return 0;
}

#else

using Ring::ByteArr;
int main(int argc, char* argv[])
{

    //  this code demonstrates how to use lib and prints all the internal data
    //  so the developer can compare and check bytes in output

    //  TODO move out debug print from library internals to users callback
    //  TODO in bright future this can be converted to unit tests

    //  use this to generate signing key pair that shoud be embedded into software
    //auto pp = Ring::GenSignPK();

    char cmdline[0xff];
    if (argc == 1) // auto test in single app
    {

        //  we use *_rx and *_tx as dummy transport layer for handshake
        //  same applies to lambdas that are passed in constructors
        ByteArr client_rx;
        ByteArr client_tx;
        mutex rx_mutex;
        mutex tx_mutex;

        Ring::SodiumGlue client(
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

        Ring::SodiumGlue server(
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
        auto msg = ByteArr{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
        debug_print("[main] msg", msg);

        auto encr = client.encrypt(msg);
        debug_print("[main] encrypted", encr);

        auto decr = server.decrypt(encr);
        debug_print("[main] decrypted", decr);

        //  this is to show how nonce is incremented internally
    //    for (int i = 0; i < 10; ++i) {
    //        auto encr = client.encrypt(msg);
    }
    else if (argc > 1 && !strcmp(argv[1], "-test"))
    {
        Ring::SodiumGlue client;
        Ring::SodiumGlue server(client.m_local_public_key);
        client.processPayload(server.m_local_public_key);

        char *msg = "@33DD";
        auto text = ByteArr(msg, msg+strlen(msg));
        debug_print("msg", text);

        auto enc = client.encrypt(text);
        debug_print("enc", enc);

        auto dec = server.decrypt(enc);
        debug_print("dec", dec);
    }
    else if (argc > 1 && !strcmp(argv[1], "-server"))
    {
        cout << "enter client PUBLIC_KEY>";
        cin >> cmdline;
        ByteArr in = Ring::SodiumGlue::convert_to_array(cmdline, true);

        Ring::SodiumGlue server(in);

        cout << "DECRYPT>";
        cin >> cmdline;
        in = Ring::SodiumGlue::convert_to_array(cmdline, true);

        auto decr = server.decrypt(in);
        char *p = Ring::SodiumGlue::deconvert_from_array(decr);
        cout << "decrypted: " << p << endl;
        delete p;


    }
    else if (argc > 1 && !strcmp(argv[1], "-client"))
    {
        Ring::SodiumGlue client;

        cout << "enter PUBLIC_PAYLOAD>";
        cin >> cmdline;
        ByteArr in = Ring::SodiumGlue::convert_to_array(cmdline, true);
        client.processPayload(in);

        cout << "ENCRYPT>";
        cin >> cmdline;

        ByteArr encin = Ring::SodiumGlue::convert_to_array(cmdline, false);
        client.encrypt(encin);

    }
}
#endif
