#ifndef HELPER_H
#define HELPER_H

#include <unistd.h>
#include <cstring>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <list>
#include <queue>

//using namespace std;

// Helper struct for blocked clients.
struct blocked_clients{
	char b_ip_addr[1024];
  	char b_host_name[1024];
  	int b_port;
  	blocked_clients():	b_port(-1){
    			  		memset(&b_ip_addr,0,sizeof(b_ip_addr));
    			  		memset(&b_host_name,0,sizeof(b_host_name));
  						}
};

// Helper struct to maintain buffer data
struct buf_data_info{
  	char buf_source[32];
  	char buf_data[1024];
  	char buf_dest_ip_addr[32];
		buf_data_info(){
    			memset(&buf_source,0,sizeof(buf_source));
    			memset(&buf_data,0,sizeof(buf_data));
    			memset(&buf_dest_ip_addr,0,sizeof(buf_dest_ip_addr));
  				}
};


// Helper struct to maintain socket data
struct socket_data_info{
  	int sock_group_id;
  	char sock_host_name[40];
  	char sock_ip[20];
 	int sock_port;
	int sock_fd;
  	int sock_rcvd_count;
  	int sock_sent_count;
  	char sock_status[16];
  	std::list<blocked_clients> blocked_list;
  	std::queue<buf_data_info> buffer;
	socket_data_info() : sock_rcvd_count(0), sock_sent_count(0), sock_port(-1), sock_group_id(-1), sock_fd(-1)
						{
   						 	memset(&sock_ip,0,sizeof(sock_ip));
   							memset(&sock_host_name,0,sizeof(sock_host_name));
    						memset(&sock_status,0,sizeof(sock_status));
  						}
};


// Helper struct to maintain some useful info
struct saved_data{
  int saved_fd;
  int saved_is;
  int saved_connected_clients_count;
  char saved_port[1024];
  char saved_ip[1024];
  std::list<blocked_clients> saved_blocked_clients;
  std::list<socket_data_info> saved_clients;
	  saved_data() :	saved_connected_clients_count(0), saved_is(1)
						{
    					memset(&saved_port,0,sizeof(saved_port));
    					memset(&saved_ip,0,sizeof(saved_ip));
  						}
};

// common.h - helper
class utils{
	protected:
  		saved_data svd;
	public:
  		void cse4589_author();
  		void cse4589_error(const char* command_str);
		void cse4589_exit();
  		void cse4589_ip();
  		void cse4589_port();
};

// server.h
class server : public utils{
	public:
  		server(char* port);
		void server_save_ip_port(char *port);
};

// client.h
class client : public utils{
	public:
  		client(char *port);
	private:
		struct hostent* client_save_ip_port(struct hostent *ht, char *port);
  		void break_cmd(const char* cmd,char *&server_ip,char *&server_port);
  		bool check_ip(char *server_ip,int port);
};


#endif

