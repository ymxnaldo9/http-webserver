#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <sys/uio.h>
#include "locker.h"
#include "utils.h"

class http_conn {
public:
    static int m_epfd;
    static int m_user_count;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    static const int FILENAME_LEN = 200;
    enum METHOD {GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT}; //http request method
    enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT};
    enum LINE_STATUS {LINE_OK = 0, LINE_BAD, LINE_OPEN};
    enum HTTP_CODE {NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION};
    http_conn() {}
    ~http_conn() {}
    void process(); //run client request
    void init(int sockfd, const sockaddr_in& addr);
    void close_conn();
    bool read();  //nonblocking read
    bool write();  //nonblocking write 
private:
    int m_sfd;  //socket connected by http
    sockaddr_in m_address; 
    char m_read_buf[READ_BUFFER_SIZE]; //read buffer
    int m_read_idx;  //the byte position next to the last byte of client data that has been read in read buffer
    int m_checked_index;
    int m_start_line;
    char* m_url;
    char* m_version;
    METHOD m_method;
    char* m_host;
    bool m_linger; //if http request keeps alive
    int m_content_length;
    char m_real_file[FILENAME_LEN];
    char* m_file_address; 
    struct stat m_file_stat;
    char m_write_buf[WRITE_BUFFER_SIZE]; // 写缓冲区
    int m_write_idx;                     // 写缓冲区中待发送的字节数
    struct iovec m_iv[2]; // 我们将采用writev来执行写操作，所以定义下面两个成员，其中m_iv_count表示被写内存块的数量。
    int m_iv_count;
    int bytes_to_send;   // 将要发送的数据的字节数
    int bytes_have_send; // 已经发送的字节数
    CHECK_STATE m_check_state;
    void init();
    HTTP_CODE process_read(); //parse http request
    HTTP_CODE parse_request_line(char* text);  //request line
    HTTP_CODE parse_headers(char* text);  //request header
    HTTP_CODE parse_content(char* text);  //request body
    LINE_STATUS parseline();
    char* get_line() { return m_read_buf + m_start_line; }
    HTTP_CODE do_request();
    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_content_type();
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
    bool process_write(HTTP_CODE ret);
};

#endif