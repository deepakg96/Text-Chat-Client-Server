/**
 * @shwetaja_assignment1
 * @author  Shweta Jana <shwetaja@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <list>
#include <algorithm>

#include "../include/global.h"
#include "../include/logger.h"
#include "../include/helper.h"

#define STD_IN 0
#define STD_OUT 1
#define BACKLOG_LISTEN 5
#define COMPARE_CHARS_BLOCKED 7
#define COMPARE_CHARS_SEND 4
#define COMPARE_CHARS_LOGIN 5
#define COMPARE_CHARS_BROADCAST 9
#define COMPARE_CHARS_UNBLOCK 7
#define COMPARE_CHARS_BLOCK 5



using namespace std;


bool compare_ports_of_clients(socket_data_info sock1, socket_data_info sock2)
{
  	return sock1.sock_port < sock2.sock_port;
}

bool compare_ports_of_blocked_clients(blocked_clients c1,blocked_clients c2)
{
 	return c1.b_port < c2.b_port;
}

bool client::check_ip(char *server_ip, int p){
  struct sockaddr_in proto;
  proto.sin_family = AF_INET;
  proto.sin_port = htons(p);
  if(inet_pton(AF_INET, server_ip, &proto.sin_addr) != 1)
    return false;
  return true;
}

void server::server_save_ip_port(char *port)
{
	strcpy(svd.saved_port, port);
	struct hostent *ht;
    char hostname[1024];
    if (gethostname(hostname,1024) < 0)
    {
        cerr<<"gethostname"<<endl;
        exit(1);
    }
    if ((ht=gethostbyname(hostname)) == NULL)
    {
        cerr<<"gethostbyname"<<endl;
        exit(1);
    }
    struct in_addr **addr_list = (struct in_addr **)ht->h_addr_list;
    for(int i = 0;addr_list[i] != NULL;++i)
    {
        strcpy(svd.saved_ip, inet_ntoa(*addr_list[i]));
    }
}

server::server(char* port){
	
	fd_set fd_set_master_serv;
    FD_ZERO(&fd_set_master_serv);
    fd_set fd_set_read_serv;
    FD_ZERO(&fd_set_read_serv);

	server_save_ip_port(port);

  	int bytes;
  	if ((svd.saved_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
    	cerr<<"Failed to create a socket\n";
    	exit(1);
  	}

  	struct sockaddr_in my_addr;
  	bzero(&my_addr,sizeof(my_addr));
  	my_addr.sin_family = AF_INET;
  	my_addr.sin_port = htons(atoi(port));
  	my_addr.sin_addr.s_addr = INADDR_ANY;
  	if (bind(svd.saved_fd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) 
	{
    	cerr<<"Failed to bind\n";
    	exit(1);
  	}

  	if (listen(svd.saved_fd, BACKLOG_LISTEN) < 0) 
	{
    	cerr<<"Failed to listen()\n";
    	exit(1);
  	}

  	struct sockaddr_in client_saddr;
  	FD_SET(svd.saved_fd,&fd_set_master_serv);
  	FD_SET(STD_IN,&fd_set_master_serv);

  	int fdmax = svd.saved_fd;
  	int newfd;
  	int addrlen;
  	char server_msg[1024];
  
  	while(1)
	{
    	fd_set_read_serv = fd_set_master_serv;
    	int ret;
    	if ((ret = select(fdmax+1, &fd_set_read_serv, NULL, NULL, NULL)) < 0) 
		{
      		cerr<<"Unable to call select()\n";
      		exit(1);
    	}
    	for(int i = 0; i <= fdmax; i++) 
		{
      		memset((void *)&server_msg,'\0',sizeof(server_msg));
      		if (FD_ISSET(i, &fd_set_read_serv)) 
			{
				if (i == svd.saved_fd)
                {
                    addrlen = sizeof(client_saddr);
                    if ((newfd = accept(svd.saved_fd, (struct sockaddr*)&client_saddr, (socklen_t*)&addrlen)) == -1)
                    {
                        cerr<<"Unable to accept\n";
                    }
                    else
                    {
                        FD_SET(newfd, &fd_set_master_serv);
                        if (newfd > fdmax)
                            fdmax = newfd;
                        }


                        bool flag = true;
                        struct sockaddr_in *sin = (struct sockaddr_in*)&client_saddr;
                        char *received_ip = inet_ntoa(sin->sin_addr);
                        for(list<socket_data_info>::iterator l1 = svd.saved_clients.begin(); l1 != svd.saved_clients.end(); ++l1)
                        {
                            if(strcmp(received_ip,l1->sock_ip) == 0)
                            {
                                strcpy(l1->sock_status,"logged-in");
                                l1->sock_fd = newfd;
                                flag = false;
                            }
                        }
                        if(flag)
                        {
                            const char *sts = "logged-in";
                            struct socket_data_info si;
                            si.sock_group_id = svd.saved_clients.size()+1;


                            struct in_addr *addr_temp;
                            struct sockaddr_in saddr;
                            struct hostent *hoste;
                            if(!inet_aton(received_ip,&saddr.sin_addr))
                            {
                                cerr<<"Failed to do inet_aton()\n";
                                exit(1);
                            }

                            if((hoste = gethostbyaddr((const void *)&saddr.sin_addr,sizeof(received_ip),AF_INET)) == NULL)
                            {
                                cerr<<"Failed to do gethostbyaddr\n";
                                exit(1);
                            }
                            char *n = hoste->h_name;
                            strcpy(si.sock_host_name,n);

                            strcpy(si.sock_ip,received_ip);
                            si.sock_fd = newfd;
                            strncpy(si.sock_status,sts,strlen(sts));

                            char port_c[8];
                            bzero(&port_c,sizeof(port_c));
                            if(recv(newfd,port_c,sizeof(port_c),0) <= 0)
                            {
                                cerr<<"Failed to do recv()\n";
                            }
                            si.sock_port = atoi(port_c);

                            svd.saved_clients.push_back(si);
                        }
						char list_message[4096];
                        bzero(&list_message,sizeof(list_message));
                        strcat(list_message,"LOGIN ");
                        for(list<socket_data_info>::iterator l2 = svd.saved_clients.begin(); l2 != svd.saved_clients.end(); ++l2)
                        {   
                            if(strcmp(l2->sock_status,"logged-in") == 0)
                            {   
                                strcat(list_message,l2->sock_host_name);
                                strcat(list_message," ");
                                strcat(list_message,l2->sock_ip);
                                strcat(list_message," ");
                                char pn[8];
                                bzero(&pn,sizeof(pn));
                                snprintf(pn, sizeof(pn), "%d", l2->sock_port);
                                strcat(list_message,pn);
                                strcat(list_message," ");
                            }   
                        }
                        for(list<socket_data_info>::iterator l3 = svd.saved_clients.begin(); l3 != svd.saved_clients.end(); ++l3)
                        {       
                            if(strcmp(l3->sock_ip,received_ip) == 0)
                            {   
                                while(!l3->buffer.empty())
                                {   
                                    buf_data_info binfo = l3->buffer.front();
                                    strcat(list_message,"BUFFER ");
                                    strcat(list_message,binfo.buf_source);
                                    strcat(list_message," ");
                        
                                    char l[5];
                                    bzero(&l,sizeof(l));
                                    int length = strlen(binfo.buf_data);
                                    sprintf(l,"%d",length);
                                    strcat(list_message,l);
                                    strcat(list_message," ");
                    
                                    strcat(list_message,binfo.buf_data);
                                    strcat(list_message," ");
                                    l3->buffer.pop();
                                    cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
                                    cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",binfo.buf_source,binfo.buf_dest_ip_addr,binfo.buf_data);
                                    cse4589_print_and_log("[%s:END]\n", "RELAYED");
                                }
                            }
                        }
                        if(send(newfd,list_message,strlen(list_message),0)<0)
                        {
                            cerr<<"Failed to send()\n";
                        }
					}
        		else if (i == STD_IN)
				{
          			read(STD_IN, server_msg, 1024);
          			server_msg[strlen(server_msg)-1]='\0';
          			
					if (strcmp(server_msg, "AUTHOR") == 0)
					{
            			cse4589_author();
          			}
          
					if (strcmp(server_msg, "IP") == 0)
					{
              			cse4589_ip();
          			}
		
					if (strcmp(server_msg,"IP") == 0)
                    {   
                        cse4589_ip();
                    }
                    	
					if (strncmp(server_msg, "BLOCKED", COMPARE_CHARS_BLOCKED) ==0 )
                    {   
                        bool valid = false;
                        char *arg[2];
                        arg[0] = strtok(server_msg, " ");
                        arg[1] = strtok(NULL, " ");
                        for(list<socket_data_info>::iterator l4 = svd.saved_clients.begin(); l4 != svd.saved_clients.end(); ++l4)
                        {   
                            if(strcmp(l4->sock_ip,arg[1]) == 0)
                            {   
                                int i = 0;
                                l4->blocked_list.sort(compare_ports_of_blocked_clients);
                                cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
                                for(list<blocked_clients>::iterator l5 = l4->blocked_list.begin(); l5 != l4->blocked_list.end(); ++l5)
                                {   
                                    cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",++i, l5->b_host_name, l5->b_ip_addr, l5->b_port);
                                }
                                cse4589_print_and_log("[BLOCKED:END]\n");
                                bool valid = true;
                            }
                        }
                        if(!valid)
                        {   
                            cse4589_error("BLOCKED");
                        }
                    }
	
					if (strcmp(server_msg, "STATISTICS") == 0)
					{
            			cse4589_print_and_log("[STATISTICS:SUCCESS]\n");
            			int i = 0;
            			svd.saved_clients.sort(compare_ports_of_clients);
            			for(list<socket_data_info>::iterator l6 = svd.saved_clients.begin(); l6 != svd.saved_clients.end(); ++l6)
						{
              				cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n",++i, l6->sock_host_name, l6->sock_sent_count, l6->sock_rcvd_count, l6->sock_status);
						}
            			cse4589_print_and_log("[STATISTICS:END]\n");
          			}
					
					if (strcmp(server_msg, "LIST") == 0)
                    {
                        cse4589_print_and_log("[LIST:SUCCESS]\n");
                        int i = 0;
                        svd.saved_clients.sort(compare_ports_of_clients);
                        for(list<socket_data_info>::iterator l7 = svd.saved_clients.begin(); l7 != svd.saved_clients.end(); ++l7)
                        {
                            if (strcmp(l7->sock_status,"logged-in") == 0)
							{
                            	cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",++i, l7->sock_host_name, l7->sock_ip, l7->sock_port);
							}
                        }   
                        cse4589_print_and_log("[LIST:END]\n");
                    }
        		}
		
				else 
				{
          			char msg[1024];
          			bzero(&msg,sizeof(msg));
          			if ((bytes = recv(i, msg, sizeof(msg), 0)) <= 0) 
					{
            			if (bytes == 0) 
						{
              				svd.saved_connected_clients_count--;
              				for(list<socket_data_info>::iterator l8 = svd.saved_clients.begin(); l8 != svd.saved_clients.end(); ++l8)
							{
                				if(l8->sock_fd == i)
								{
                  					const char *stso = "logged-out";
                  					strncpy(l8->sock_status, stso, strlen(stso));
                				}
              				}
            			}	
            			else 
						{
              				cerr<<"recv";
            			}
           				close(i);
            			FD_CLR(i, &fd_set_master_serv); 
          			}
          			else 
					{
            			char original_msg[1024];
            			char buffer_msg[1024];
            			bzero(&original_msg,sizeof(original_msg));
            			bzero(&buffer_msg,sizeof(buffer_msg));
            			strcpy(original_msg,msg);
            			strcpy(buffer_msg,msg);
            			char *arg_zero = strtok(msg," ");


            			if(strcmp(arg_zero,"SEND") == 0)
						{
             	 			char from_client_ip[32];
              				bzero(&from_client_ip,sizeof(from_client_ip));
              				for(list<socket_data_info>::iterator l9 = svd.saved_clients.begin(); l9 != svd.saved_clients.end(); ++l9)
							{
                				if(l9->sock_fd == i)
								{
                  					strcpy(from_client_ip, l9->sock_ip);
                  					l9->sock_sent_count++;
                				}
							}
              				char *arg[3];
              				arg[1] = strtok(NULL," ");
              				arg[2] = strtok(NULL,"");

              				char new_msg[1024];
              				bzero(&new_msg,sizeof(new_msg));
              				strcat(new_msg,"SEND ");
              				strcat(new_msg,(const char*) from_client_ip);
              				strcat(new_msg," ");
              				strcat(new_msg,arg[1]);
              				strcat(new_msg," ");
              				strcat(new_msg,arg[2]);

              				bool blocked = false;
              				bool log = true;
              				for(list<socket_data_info>::iterator l10 = svd.saved_clients.begin(); l10 != svd.saved_clients.end(); ++l10)
							{
                				if(strcmp(l10->sock_ip,arg[1]) == 0)
								{
                  					if(strcmp(l10->sock_status,"logged-out") == 0)
									{
                    					log = false;
                  					}
                  					for(list<blocked_clients>::iterator l11 = l10->blocked_list.begin(); l11 != l10->blocked_list.end(); ++l11)
									{
                    					if(strcmp(l11->b_ip_addr, from_client_ip) == 0)
                     					blocked = true;
                  					}
                				}
              				}

              				if(log && !blocked)
							{
                				cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
               	 				cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",from_client_ip,arg[1],arg[2]);
                				cse4589_print_and_log("[%s:END]\n", "RELAYED");
                				for(list<socket_data_info>::iterator l12 = svd.saved_clients.begin(); l12 != svd.saved_clients.end(); ++l12)
								{
                  					if(strcmp(l12->sock_ip,arg[1]) == 0)
									{
                    					if(send(l12->sock_fd,new_msg,strlen(new_msg),0)<0)
										{
                     	 					cerr<<"Failed to send()\n";
                    					}
                    					l12->sock_rcvd_count++;
                    					break;
                  					}
                				}
                				bzero(&msg,sizeof(msg));
              				}
			
              				if(!log && !blocked)
							{
								buf_data_info bi;
								strcpy(bi.buf_dest_ip_addr,arg[1]);
                				strcpy(bi.buf_data,arg[2]);
                				strcpy(bi.buf_source,from_client_ip);
								for(list<socket_data_info>::iterator l13 = svd.saved_clients.begin(); l13 != svd.saved_clients.end(); ++l13)
								{
                  					if(strcmp(l13->sock_ip,arg[1]) == 0)
									{
                    					l13->buffer.push(bi);
                    					l13->sock_rcvd_count++;
                  					}
                				}
              				}
            			}
		 	
						else if(strcmp(arg_zero,"REFRESH") == 0)
                        {
                            char list_message[4096];
                            bzero(&list_message,sizeof(list_message));
                            char *received_ip = strtok(NULL," ");
                            strcat(list_message,"REFRESH ");
                            for(list<socket_data_info>::iterator l14 = svd.saved_clients.begin(); l14 != svd.saved_clients.end(); ++l14)
                            {
                                if(strcmp(l14->sock_status,"logged-in") == 0)
                                {
                                    strcat(list_message,l14->sock_host_name);
                                    strcat(list_message," ");
                                    strcat(list_message,l14->sock_ip);
                                    strcat(list_message," ");
                                    char pn[8];
                                    bzero(&pn,sizeof(pn));
                                    snprintf(pn, sizeof(pn), "%d", l14->sock_port);
                                    strcat(list_message,pn);
                                    strcat(list_message," ");
                                }
                            }
                            if(send(i,list_message,strlen(list_message),0)<0)
                            {
                                cerr<<"Failed to send()\n";
                            }
                        }
						
						else if(strcmp(arg_zero,"BLOCK") == 0)
						{
              				char *arg[2];
              				arg[1] = strtok(NULL," ");
              				for(list<socket_data_info>::iterator l15 = svd.saved_clients.begin(); l15 != svd.saved_clients.end(); ++l15)
							{
                				if(l15->sock_fd == i)
								{
                  					blocked_clients b;
                  					strcpy(b.b_ip_addr,arg[1]);
                  					for(list<socket_data_info>::iterator l16 = svd.saved_clients.begin(); l16 != svd.saved_clients.end(); ++l16)
									{
                    					if(strcmp(l16->sock_ip,arg[1]) == 0)
										{
                      						b.b_port = l16->sock_port;
                      						strcpy(b.b_host_name, l16->sock_host_name);
                    					}
                  					}
                  					l15->blocked_list.push_back(b);
                				}
              				}
            			}
            
						else if(strcmp(arg_zero,"UNBLOCK") == 0)
						{
              				char *arg[2];
              				arg[1] = strtok(NULL," ");
              				for(list<socket_data_info>::iterator l16 = svd.saved_clients.begin(); l16 != svd.saved_clients.end(); ++l16)
							{
                				if(l16->sock_fd == i)
								{
                  					for(list<blocked_clients>::iterator l17 = l16->blocked_list.begin(); l17 != l16->blocked_list.end(); ++l17)
									{
                    					if(strcmp(arg[1], l17->b_ip_addr) == 0)
										{
                      						l16->blocked_list.erase(l17);
										}
									}
                				}
              				}
            			}
						
						else if(strcmp(arg_zero,"BROADCAST") == 0)
                        {   
                            cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
                            char from_client_ip[32];
                            bzero(&from_client_ip,sizeof(from_client_ip));
                            for(list<socket_data_info>::iterator l18 = svd.saved_clients.begin(); l18 != svd.saved_clients.end(); ++l18)
                            {   
                                if(l18->sock_fd == i)
                                {   
                                    strcpy(from_client_ip, l18->sock_ip);
                                    l18->sock_sent_count++;
                                }
                            }
                            char *arg[2];
                            arg[1] = strtok(NULL,"");
                            
                            char new_msg[1024];
                            bzero(&new_msg,sizeof(new_msg));
                            strcat(new_msg,"BROADCAST ");
                            strcat(new_msg,from_client_ip);
                            strcat(new_msg," ");
                            strcat(new_msg,arg[1]);
                            
                            cse4589_print_and_log("msg from:%s, to:255.255.255.255\n[msg]:%s\n",from_client_ip,arg[1]);
                            cse4589_print_and_log("[%s:END]\n", "RELAYED");
                            for(list<socket_data_info>::iterator l19 = svd.saved_clients.begin(); l19 != svd.saved_clients.end(); ++l19)
                            {   
                                if(l19->sock_fd != i && strcmp(l19->sock_status,"logged-in") == 0)
                                {   
                                    if(send(l19->sock_fd,new_msg,strlen(new_msg),0)<0)
                                    {   
                                        cse4589_error("BROADCAST");
                                    }
                                    l19->sock_rcvd_count++;
                                }
                                if(l19->sock_fd != i && strcmp(l19->sock_status,"logged-out") == 0)
                                {   
                                    buf_data_info binfo;
                                    strcpy(binfo.buf_dest_ip_addr,l19->sock_ip);
                                    strcpy(binfo.buf_data,arg[1]);
                                    strcpy(binfo.buf_source,from_client_ip);
                                    l19->buffer.push(binfo);
                                    l19->sock_rcvd_count++;
                                }
                            }
                        }
          			}
        		}
      		}
    	}
  	}
}

struct hostent* client::client_save_ip_port(struct hostent *ht, char *port)
{
	strcpy(svd.saved_port,port);
	
	char host_name[1024];
    if (gethostname(host_name,1024) < 0)
    {
        cerr<<"Failed to do gethostname()\n";
        exit(1);
    }

    if ((ht=gethostbyname(host_name)) == NULL)
    {
        cerr<<"Failed to do gethostbyname()\n";
        exit(1);
    }
    struct in_addr **addr_list = (struct in_addr **)ht->h_addr_list;
    for(int i = 0;addr_list[i] != NULL;++i)
    {
        strcpy(svd.saved_ip,inet_ntoa(*addr_list[i]));
    }
	return ht;

}
client::client(char *port)
{
	struct hostent *host_ent;
	host_ent = client_save_ip_port(host_ent, port);
  	
	if ((svd.saved_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
    	cerr<<"Failed to get socket()\n";
    	exit(1);
  	}

  	struct sockaddr_in client_addr;
  	bzero(&client_addr,sizeof(client_addr));
  	client_addr.sin_family = AF_INET;
  	client_addr.sin_port = htons(atoi(port));
  	client_addr.sin_addr = *((struct in_addr*)host_ent->h_addr);
  	if (bind(svd.saved_fd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) 
	{
    	cerr<<"Failed to do bind()\n";
    	exit(1);
  	}


  	char client_msg[1024];
  	while(1)
	{
    	bzero(&client_msg, sizeof(client_msg));

    	read(STD_IN, client_msg, 1024);
    	client_msg[strlen(client_msg)-1]='\0';
	
		if (strcmp(client_msg,"AUTHOR") == 0)
        {
            cse4589_author();
        }

		else if (strcmp(client_msg,"IP") == 0)
        {
            cse4589_ip();
        }	

    	else if (strcmp(client_msg,"EXIT") == 0)
		{
			cse4589_exit();
			close(svd.saved_fd);
      		exit(0);
    	}
	
		else if (strcmp(client_msg,"PORT") == 0)
		{
      		cse4589_port();
    	}
    
		else if (strcmp(client_msg,"LIST") == 0)
		{
      		int list_k = 0;
      		svd.saved_clients.sort(compare_ports_of_clients);
      		cse4589_print_and_log("[LIST:SUCCESS]\n");
      		for(list<socket_data_info>::iterator c1 = svd.saved_clients.begin(); c1 != svd.saved_clients.end(); ++c1)
			{
        		if (strcmp(c1->sock_status,"logged-in") == 0)
				{			
           			cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", ++list_k, c1->sock_host_name, c1->sock_ip, c1->sock_port);
				}
      		}
      		cse4589_print_and_log("[LIST:END]\n");
    	}
	
		else if (strncmp(client_msg,"LOGIN",COMPARE_CHARS_LOGIN) == 0)
		{
      		char *ip_serv;
      		char *port_serv;
      		strtok(client_msg," ");
      		ip_serv = strtok(NULL," ");
      		port_serv = strtok(NULL," ");


      		bool is_port_valid = true;
			int port = atoi(port_serv);
            if(port < 0 || port > 65535)
            {
                cse4589_error("LOGIN");
                continue;
            }

      		if(port_serv == NULL)
			{
        		cse4589_error("LOGIN");
        			continue;
      		}
      		for(int j = 0; j != strlen(port_serv); ++j)
			{
        		if(port_serv[j] >= '0' && port_serv[j] <= '9')
				{
          			continue;
        		}
        		else
				{
          			cse4589_error("LOGIN");
          			is_port_valid = false;
          			break;
        		}
      		}
      		if(!is_port_valid)
			{
				cse4589_error("LOGIN"); 
				continue;
			}

      		if (!check_ip(ip_serv ,port))
			{
        		cse4589_error("LOGIN");
        		continue;
      		}
			else
			{
        		struct addrinfo *target;
        		struct addrinfo ainfo;
        		bzero(&ainfo, sizeof(ainfo));
        		ainfo.ai_socktype = SOCK_STREAM;
        		ainfo.ai_family = AF_INET;
        		if (getaddrinfo(ip_serv, port_serv, &ainfo, &target) != 0) 
				{
          			cse4589_error("LOGIN");
          			continue;
        		}
        		else
				{
          			if ((svd.saved_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
					{
            			cse4589_error("LOGIN");
            			continue;
          			}

          			struct sockaddr_in tgt;
         			bzero(&tgt,sizeof(tgt));
          			tgt.sin_port = htons(port);
          			tgt.sin_addr.s_addr = inet_addr(ip_serv);
          			tgt.sin_family = AF_INET;
          			if ((connect(svd.saved_fd, (struct sockaddr *)&tgt, sizeof(struct sockaddr))) < 0)
					{
            			cse4589_error("LOGIN");
            			continue;
          			}

          			char cp[8];
          			bzero(&cp,sizeof(cp));
          			strcat(cp,svd.saved_port);
          			if(send(svd.saved_fd,cp,strlen(cp),0)<0)
					{
            			cerr<<"Send() failed\n";
          			}
					for(;;)
					{
						int high_fdes = svd.saved_fd;
            			fd_set c_fd_set;
            			FD_ZERO(&c_fd_set);
            			bzero(&client_msg, sizeof(client_msg));
            			FD_SET(STD_IN, &c_fd_set);
            			FD_SET(svd.saved_fd, &c_fd_set);

            			select(high_fdes+1, &c_fd_set, NULL, NULL, NULL);
            			if (FD_ISSET(STD_IN, &c_fd_set))
						{
              				read(STD_IN,client_msg,1024);
              				client_msg[strlen(client_msg)-1]='\0';

              				if (strcmp(client_msg,"AUTHOR") == 0)
							{
                				cse4589_author();
              				}
	
              				else if (strcmp(client_msg,"PORT") == 0)
							{
                				cse4589_port();
              				}
	
              				else if (strcmp(client_msg,"IP") == 0)
							{
                				cse4589_ip();
              				}

							else if(strcmp(client_msg,"REFRESH") == 0)
							{
                				strcat(client_msg, " ");
                				strcat(client_msg, svd.saved_ip);
                				if(send(svd.saved_fd, client_msg, strlen(client_msg), 0)<0)
								{
                  					cse4589_error("REFRESH");
                				}
               				 	cse4589_print_and_log("[REFRESH:SUCCESS]\n");
                				cse4589_print_and_log("[REFRESH:END]\n");
              				}
		
							
							else if (strcmp(client_msg,"LIST") == 0)
                            {
                                svd.saved_clients.sort(compare_ports_of_clients);
                                int lk_c = 0;
                                cse4589_print_and_log("[LIST:SUCCESS]\n");
                                for(list<socket_data_info>::iterator c2 = svd.saved_clients.begin(); c2 != svd.saved_clients.end(); ++c2)
                                {
                                    if (strcmp(c2->sock_status,"logged-in") == 0)
                                    {
                                        cse4589_print_and_log("%-5d%-35s%-20s%-8d\n",++lk_c, c2->sock_host_name, c2->sock_ip, c2->sock_port);
                                    }
                                }
                                cse4589_print_and_log("[LIST:END]\n");
                            }
							
							else if(strncmp(client_msg,"SEND",COMPARE_CHARS_SEND) == 0)
							{
                				bool send_sanity = false;
                				char *parse_st[3];
                				char sendtoclient[1024];
               				 	bzero(&sendtoclient,sizeof(sendtoclient));
                				strcpy(sendtoclient,client_msg);
                				parse_st[0] = strtok(client_msg," ");
                				for(int i = 1; i!=3; ++i)
								{
                  					parse_st[i] = strtok(NULL," ");
                				}
                
                				for(list<socket_data_info>::iterator c3 = svd.saved_clients.begin(); c3 != svd.saved_clients.end(); ++c3)
								{
                  					if(strcmp(c3->sock_ip,parse_st[1]) == 0)
                    				{
										send_sanity = true;
									}
                				}

                				if(!send_sanity || send(svd.saved_fd,sendtoclient,strlen(sendtoclient),0)<0)
								{
                  					cse4589_error("SEND");
                  					continue;
                				}
                				cse4589_print_and_log("[SEND:SUCCESS]\n");
                				cse4589_print_and_log("[SEND:END]\n");
              				}
              
							else if(strncmp(client_msg,"BROADCAST",COMPARE_CHARS_BROADCAST) == 0)
							{
                				if(send(svd.saved_fd,client_msg,strlen(client_msg),0)<0)
								{
                  					cse4589_error("BROADCAST");
                				}
                				cse4589_print_and_log("[BROADCAST:SUCCESS]\n");
                				cse4589_print_and_log("[BROADCAST:END]\n");
              				}
			
							else if(strncmp(client_msg,"BLOCK", COMPARE_CHARS_BLOCK) == 0)
							{
                				bool flag_blocked = false;
                				bool flag_b = false;
                				char args_parse[1024];
								blocked_clients cli;
                				bzero(&args_parse, sizeof(args_parse));
                				strcpy(args_parse, client_msg);
                				strtok(args_parse, " ");
                				char *block_ip = strtok(NULL," ");
                				
                				for(list<socket_data_info>::iterator c4 = svd.saved_clients.begin(); c4 != svd.saved_clients.end(); ++c4)
								{
                  					if(strcmp(c4->sock_ip,block_ip) == 0) 
									{
                    					flag_b = true;
                    					cli.b_port = c4->sock_port;
                    					strcpy(cli.b_host_name,c4->sock_host_name);
                    					strcpy(cli.b_ip_addr,c4->sock_ip);
                    					break;
                  					}
                				}
                				for(list<blocked_clients>::iterator c5 = svd.saved_blocked_clients.begin(); c5 != svd.saved_blocked_clients.end(); ++c5)
								{
                  					if(strcmp(block_ip,c5->b_ip_addr) == 0)
									{
                    					flag_blocked = true;
                    					break;
                  					}
                				}
								if(flag_blocked || !flag_b)
								{
                  					cse4589_error("BLOCK");
                  					continue;
                				}
                				if(send(svd.saved_fd,client_msg,strlen(client_msg),0)<0)
								{
                  					cse4589_error("BLOCK");
                  					continue;
                				}
                				else
								{
                  					svd.saved_blocked_clients.push_back(cli);
                				}
                				cse4589_print_and_log("[BLOCK:SUCCESS]\n");
                				cse4589_print_and_log("[BLOCK:END]\n");
              				}
             				
							else if(strcmp(client_msg,"EXIT") == 0)
                            {
								cse4589_exit();
                                close(svd.saved_fd);
                                exit(0);
                            }

							else if(strncmp(client_msg,"UNBLOCK",COMPARE_CHARS_UNBLOCK) == 0)
							{
                				char msg_ub[1024];
                				char *parse_ub[2];
                				bzero(&msg_ub,sizeof(msg_ub));
                				strcpy(msg_ub,client_msg);
                				bool flag_ub = false;
                				parse_ub[0] = strtok(msg_ub," ");
                				parse_ub[1] = strtok(NULL," ");
                				for(list<blocked_clients>::iterator c6 = svd.saved_blocked_clients.begin(); c6 != svd.saved_blocked_clients.end(); ++c6)
								{
                  					if(strcmp(c6->b_ip_addr,parse_ub[1]) == 0)
									{
										flag_ub = true;
                    					svd.saved_blocked_clients.erase(c6);
                    					break;
                  					}
                				}
                				if((send(svd.saved_fd,client_msg,strlen(client_msg),0)<0) || !flag_ub)
								{
                  					cse4589_error("UNBLOCK");
                  					continue;
                				}
                				cse4589_print_and_log("[UNBLOCK:SUCCESS]\n");
                				cse4589_print_and_log("[UNBLOCK:END]\n");
              				}
			
							else if(strcmp(client_msg,"LOGOUT") == 0)
							{
                				cse4589_print_and_log("[LOGOUT:SUCCESS]\n");
                				close(svd.saved_fd);
                				cse4589_print_and_log("[LOGOUT:END]\n");
               	 				break;
              				}

            			}
            			
						else
						{
							char msg[1024];
                            bzero(&msg,sizeof(msg));
                            int recvbytes;
                            if((recvbytes = recv(svd.saved_fd,msg,sizeof(msg),0)) <= 0)
                            {
                                cerr<<"Failed to do recv()\n";
                            }

                            char *arg_zero = strtok(msg," ");
							if(FD_ISSET(svd.saved_fd,&c_fd_set))
                            {
                                if(strcmp(arg_zero,"LOGIN") == 0)
                                {
                                    svd.saved_clients.clear();
                                    while(true)
                                    {
                                        char *list_msg[3];

                                        list_msg[0] = strtok(NULL," ");
                                        char mesg[512];
                                        char messag[4096];
                                        bzero(&messag,sizeof(messag));
                                        while(list_msg[0] != NULL && strcmp(list_msg[0],"BUFFER") == 0)
                                        {
                                            char original_messag[4096];
                                            bzero(&original_messag,sizeof(original_messag));
                                            strcpy(messag,strtok(NULL,""));
                                            strcpy(original_messag,messag);

                                            char *fr = strtok(original_messag," ");
                                            char *l = strtok(NULL," ");

                                            int length = atoi(l);
                                            char *next;
                                            next = strtok(NULL,"");
                                            bzero(&mesg,sizeof(mesg));
                                            strncpy(mesg,next,length);
                                            cse4589_print_and_log("[%s:SUCCESS]\n", "RECEIVED");
                                            cse4589_print_and_log("msg from:%s\n[msg]:%s\n",fr,mesg);
                                            cse4589_print_and_log("[%s:END]\n", "RECEIVED");

                                            list_msg[0] = strtok(messag," ");
                                            if(strcmp(list_msg[0],"BUFFER") == 0 || list_msg[0] == NULL)
                                            {
                                                continue;
                                            }
											while((list_msg[0] = strtok(NULL," ")) != NULL && strcmp(list_msg[0],"BUFFER") != 0)
                                            {
                                                continue;
                                            }
                                        }
                                        if(list_msg[0] == NULL)
                                        {
                                            cse4589_print_and_log("[LOGIN:SUCCESS]\n");
                                            cse4589_print_and_log("[LOGIN:END]\n");
                                            break;
                                        }
                                        for(int j = 1;j != 3;++j)
                                        {
                                            bzero(&list_msg[j],sizeof(list_msg[j]));
                                            list_msg[j] = strtok(NULL," ");
                                        }
                                        struct socket_data_info si;
                                        strcpy(si.sock_host_name,list_msg[0]);
                                        strcpy(si.sock_ip,list_msg[1]);
                                        int port_n = atoi(list_msg[2]);
                                        si.sock_port = port_n;
                                        strcpy(si.sock_status,"logged-in");
                                        svd.saved_clients.push_back(si);
                                    }
                                }
								else if(strcmp(arg_zero,"SEND") == 0)
                                {
                                    cse4589_print_and_log("[%s:SUCCESS]\n", "RECEIVED");
                                    char *arg[4];
                                    arg[1] = strtok(NULL," ");
                                    arg[2] = strtok(NULL," ");
                                    arg[3] = strtok(NULL,"");
                                    cse4589_print_and_log("msg from:%s\n[msg]:%s\n",arg[1],arg[3]);
                                    cse4589_print_and_log("[%s:END]\n", "RECEIVED");
                                }
                                else if(strcmp(arg_zero,"REFRESH") == 0)
                                {
                                    svd.saved_clients.clear();
                                    while(true)
                                    {
                                        char *list_msg[3];
                                        if((list_msg[0] = strtok(NULL," ")) == NULL)
                                        {
                                            break;
                                        }
                                        for(int j = 1;j != 3;++j)
                                        {
                                            bzero(&list_msg[j],sizeof(list_msg[j]));
                                            list_msg[j] = strtok(NULL," ");
                                        }
                                        struct socket_data_info sock_dat;
                                        strcpy(sock_dat.sock_host_name,list_msg[0]);
                                        strcpy(sock_dat.sock_ip,list_msg[1]);
                                        int port_n = atoi(list_msg[2]);
                                        sock_dat.sock_port = port_n;
                                        strcpy(sock_dat.sock_status,"logged-in");
                                        svd.saved_clients.push_back(sock_dat);
                                    }
                                }
								else if(strcmp(arg_zero,"BROADCAST") == 0)
                                {
                                    cse4589_print_and_log("[%s:SUCCESS]\n", "RECEIVED");
                                    char *arg[3];
                                    arg[1] = strtok(NULL," ");
                                    arg[2] = strtok(NULL,"");
                                    cse4589_print_and_log("msg from:%s\n[msg]:%s\n",arg[1],arg[2]);
                                    cse4589_print_and_log("[%s:END]\n", "RECEIVED");
                                }
							}
            			} 
          			}
        		}
      		}
    	}
  	}
}

void utils::cse4589_error(const char* cmd){
  	cse4589_print_and_log("[%s:ERROR]\n",cmd);
  	cse4589_print_and_log("[%s:END]\n",cmd);
}

void utils::cse4589_exit()
{
	cse4589_print_and_log("[EXIT:SUCCESS]\n");
    cse4589_print_and_log("[EXIT:END]\n");
}

void utils::cse4589_author(){
  	cse4589_print_and_log("[AUTHOR:SUCCESS]\n");
  	cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", "shwetaja");
  	cse4589_print_and_log("[AUTHOR:END]\n");
}

void utils::cse4589_ip(){
  	cse4589_print_and_log("[IP:SUCCESS]\n");
  	cse4589_print_and_log("IP:%s\n",svd.saved_ip);
  	cse4589_print_and_log("[IP:END]\n");
}

void utils::cse4589_port(){
  	cse4589_print_and_log("[PORT:SUCCESS]\n");
  	cse4589_print_and_log("PORT:%s\n",svd.saved_port);
  	cse4589_print_and_log("[PORT:END]\n");
}

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	// This specifies the executable always has 3 args. i.e executable itself, s or c indicating server or client and port number.
	if (argc != 3)
	{
    	return 0;
	}
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/* Clear LOGFILE*/
    fclose(fopen(LOGFILE, "w"));

	/*Start Here*/
   	if (strcmp(argv[1],(char*)"s") == 0)
	{
		server sobj(argv[2]);
	}
	else
	{
    	client cobj(argv[2]);
	}
	return 0;
}
