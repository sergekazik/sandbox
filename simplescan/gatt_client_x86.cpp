#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

// static definitions
static const char* gsDefaultCondifg =
"{\n" \
"   \"config\":{\n" \
"       \"gatt\":{\n" \
"           \"device_name\":\"RING-7E01\",\n" \
"           \"service_uuid\":\"0000FACE-0000-1000-8000-00805F9B34FB\",\n" \
"           \"connect\":\"auto\",\n" \
"           \"ssid\":\"ring_wifi_test_ssid\",\n" \
"           \"pass\":\"ring_wifi_test_password\",\n" \
"           \"scan_timeout\":\"18\",\n" \
"           \"payload_ready_timeout\":\"5\",\n" \
"           \"set_network_timeout\":\"10\",\n" \
"           \"characteristics\": {\n" \
"               \"SET_PUBLIC_KEY\":\"0000FACE-0000-1000-8000-00805F9B3501\",\n" \
"               \"GET_PUBLIC_PAYLOAD\":\"0000FACE-0000-1000-8000-00805F9B3502\",\n" \
"               \"GET_NETWORKS\":\"0000FACE-0000-1000-8000-00805F9B3503\",\n" \
"               \"SET_NETWORK\":\"0000FACE-0000-1000-8000-00805F9B3504\",\n" \
"               \"GET_PAIRING_STATE\":\"0000FACE-0000-1000-8000-00805F9B3507\",\n" \
"               \"SET_LANGUAGE\":\"0000FACE-0000-1000-8000-00805F9B3508\",\n" \
"               \"SET_ZIPCODE\":\"0000FACE-0000-1000-8000-00805F9B3509\",\n" \
"               \"GET_WIFI_STATUS\":\"0000FACE-0000-1000-8000-00805F9B350A\",\n" \
"               \"GET_SSID_WIFI\":\"0000FACE-0000-1000-8000-00805F9B350B\",\n" \
"               \"GET_SERIAL_NUMBER\":\"0000FACE-0000-1000-8000-00805F9B350C\",\n" \
"               \"GET_MAC_ADDRESS\":\"0000FACE-0000-1000-8000-00805F9B350D\",\n" \
"           }\n" \
"       }\n" \
"       \"pairing\":{\n" \
"           \"SET_LANGUAGE\":\"eng\",\n" \
"           \"SET_ZIPCODE\":\"90404\",\n" \
"           \"GET_WIFI_STATUS\":\"CONNECTED\",\n" \
"           \"GET_SSID_WIFI\":\"ssid\",\n" \
"           \"GET_SERIAL_NUMBER\":\"-report-\",\n" \
"           \"GET_MAC_ADDRESS\":\"-report-\",\n" \
"       }\n" \
"   }\n" \
"}\n";


// Helper and Configuration functions
void getValByKeyfromJson(const char* json_str, const char* key, char* val, int len)
{
    if (json_str && key && val) {
        memset(val, 0, len);
        char *pStart = (char*) strstr(json_str, key);
        if (pStart) {
            pStart += strlen(key)+3; //3 for ":"
            char *pEnd = (char*) strchr(pStart, '"');
            if (pEnd && (pEnd > pStart)) {
                int ln = pEnd-pStart+1;
                if (ln > len) ln = len;
                strncpy(val, pStart, ln-1);
            }
        }
    }
}


int main(int argc, char** argv)
{
    (void) argc;

    char config_buffer[0xFFF];
    char filename[0xff];
    sprintf(filename, "%s.cfg", argv[0]);

    FILE* fin = fopen(filename, "rb");
    if (!fin)
    {
        // create default
        fin= fopen(filename,"w");
        if (!fin)
        {
            printf("ERROR: open file to write %s\n", filename);
            return -1;
        }
        fwrite(gsDefaultCondifg, 1, strlen(gsDefaultCondifg), fin);
        fclose(fin);

        // reopen for read
        fin = fopen(filename, "rb");
        if (!fin)
        {
            printf("ERROR: open file to read %s\n", filename);
            return -2;
        }
    }

    if (fread(config_buffer, 1, 0xFFF, fin))
        printf("%s", config_buffer);
    return 0;
}
