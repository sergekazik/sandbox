#ifndef CLIENTIPDETECTOR_H
#define CLIENTIPDETECTOR_H

#include "pb_common.h"

namespace Ring {

class ClientIPDetector
{
public:
    ClientIPDetector();
    ~ClientIPDetector();

    error_code_t init_response_info(const char *buffer);
    error_code_t parse_script_command(const char *line);
    error_code_t preprocess_script_command();
    error_code_t connect_tcp_server(char *line);
    error_code_t process_server_response();

    void handle_sendto_command();
    void handle_test_udp_command();
    void printf_cci_info(const char *command);

    control_channel_info_t  m_cci;
    control_channel_info_t *m_rsp_cci = nullptr;
    char m_server_ip[16] = "127.0.0.1";
    int m_ctrl_sock = INVALID_SOCKET;
    int m_test_sock = INVALID_SOCKET;
    int m_test_port = 0;
    int m_sock_hndl = 0;

private:
    bool check_socket(int &sock, int type);


};

} // namespace Ring

#endif // CLIENTIPDETECTOR_H
