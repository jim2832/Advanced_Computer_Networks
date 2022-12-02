#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <time.h>
#include <net/if.h>

#include "fill_packet.h"
#include "pcap.h"


#define IP_SIZE 16
#define req_size 50
#define ADDRLEN 30


pid_t pid;
u16 icmp_req = 1;
struct timeval stop, start, middle;

void print_usage()
{
	printf("Usage\n");
	printf("sudo ./ipscanner -i [Network Interface Name] -t [timeout(ms)]\n");
}

int main(int argc, char* argv[])
{
	int sockfd;
	int on = 1;
	int sockfd_send;
	
	pid = getpid();
	struct sockaddr_in dst;
	
	struct in_addr myip, mymask;
	struct ifreq req_local; 
	char device_name[100];
	
	myicmp packet;
	int timeout = DEFAULT_TIMEOUT;

	strcpy(device_name, argv[2]);
	strcpy(req_local.ifr_name, device_name);
	timeout = atoi(argv[4]);

	//determin the root status
	if(geteuid() != 0){
			printf("%s\n","ERROR: You must be root to use this tool!");
			exit(1);
	}

	//set socket
	if((sockfd_send = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0)
	{
			perror("open send socket error");
			exit(1);
	}
	
	/* get ip address of my interface */
    if(ioctl(sockfd_send, SIOCGIFADDR, &req_local) < 0) {
        perror("ioctl SIOCGIFADDR error");
        myip.s_addr = 0;
    }
    else {
        memcpy(&dst, &req_local.ifr_addr, sizeof(dst));
        myip = dst.sin_addr;
    }

	 /*get network mask of my interface */
	if( ioctl(sockfd_send, SIOCGIFNETMASK, &req_local) == -1){
		perror("SIOCGIFADDR ERROR");
		exit(1);
		mymask.s_addr = 0;
	}
	else{
		memcpy(&dst,&req_local.ifr_addr,sizeof(dst));
        mymask = dst.sin_addr;
	}
	
	char IP[INET_ADDRSTRLEN];
	char mask[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &myip, IP, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &mymask, mask, INET_ADDRSTRLEN);

	// printf("myip = %s \n", IP);
	// printf("mymask = %s \n", mask);
	
	//split the mask
	char temp_mask[ADDRLEN];
	unsigned char splited_mask[ADDRLEN]; //store subnet mask each value (int)
	memcpy(temp_mask, mask, ADDRLEN);
	char *Mask_token;
	int MASK_Num;
	Mask_token = strtok(temp_mask, ".");
	int i=0;
	while( Mask_token != NULL) 
	{
		MASK_Num = atoi(Mask_token);
		splited_mask[i] = MASK_Num;
		i++;
		Mask_token = strtok(NULL,".");
	}
	
	//split the ip address
	char temp_ip[ADDRLEN];
	unsigned char 	splited_ip[ADDRLEN]; //store subnet mask each value (int)
	memcpy(temp_ip, IP, ADDRLEN);
	char *IP_token;
	int IP_Num;
	IP_token = strtok(temp_ip, ".");
	int j=0;
	while( IP_token != NULL) 
	{
		IP_Num = atoi(IP_token);
		splited_ip[j] = IP_Num;
		j++;
		IP_token = strtok(NULL,".");
	}
	

	int available_ip,segment,start_ip,end_ip;
	if(splited_mask[2] == 255){
		
		available_ip = 256 - splited_mask[3];
		segment = 256 / available_ip;

		//printf("available_ip :%d\n",available_ip);
		//printf("segment :%d\n",segment);
		if(segment == 1){
			start_ip =0+1;
			end_ip = 255-1;
		}
		else if(segment == 2){
			if( splited_ip[3]<128){
				start_ip =0+1;
				end_ip = 128-1;
			}
			else{
				start_ip =128;
				end_ip = 255-1;
			}
		}
		else if(segment == 4){
			if(splited_ip[3]<64){
				start_ip =0+1;
				end_ip = 63-1;
			}
			else if(splited_ip[3]>63 && splited_ip[3]<128){
				start_ip =64+1;
				end_ip = 127-1;
			}
			else if(splited_ip[3]>127 && splited_ip[3]<192){
				start_ip =128+1;
				end_ip = 191-1;
			}
			else if(splited_ip[3]>191 && splited_ip[3]<256){
				start_ip =191+1;
				end_ip = 255-1;
			}
		}
		else if(segment == 8){
			if(splited_ip[3]<32){
				start_ip =0+1;
				end_ip = 31-1;
			}
			else if(splited_ip[3]>31 && splited_ip[3]<64){
				start_ip =32+1;
				end_ip = 63-1;
			}
			else if(splited_ip[3]>63 && splited_ip[3]<96){
				start_ip =64+1;
				end_ip = 95-1;
			}
			else if(splited_ip[3]>95 && splited_ip[3]<128){
				start_ip =96+1;
				end_ip = 127-1;
			}
			else if(splited_ip[3]>127 && splited_ip[3]<160){
				start_ip =128+1;
				end_ip = 159-1;
			}
			else if(splited_ip[3]>159 && splited_ip[3]<192){
				start_ip =160+1;
				end_ip = 191-1;
			}
			else if(splited_ip[3]>191 && splited_ip[3]<224){
				start_ip =192+1;
				end_ip = 223-1;
			}
			else if(splited_ip[3]>223 && splited_ip[3]<256){
				start_ip =224+1;
				end_ip = 255-1;
			}
		}
	}
	//printf("%d\n",start_ip);
	//printf("%d\n",end_ip );

	if(argc == 5){
		if(!strcmp(argv[0],"./ipscanner") && !strcmp(argv[1],"-i") && !strcmp(argv[3],"-t"))
		{	
			

			for(int i=start_ip;i<=end_ip;i++){
				char testIP[30];
				sprintf(testIP,"%d.%d.%d.%d",splited_ip[0],splited_ip[1],splited_ip[2],i);
				if((sockfd = socket(AF_INET, SOCK_RAW , IPPROTO_RAW)) < 0)
				{
					perror("socket");
					exit(1);
				}
				if(setsockopt( sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
				{
					perror("setsockopt");
					exit(1);
				}

				char data[20] = "M073040023";
				dst.sin_family = AF_INET;
				//memcpy(testIP,"10.0.2.2",sizeof(testIP));
				dst.sin_addr.s_addr = inet_addr(testIP);
				
				printf("Ping %s (data size = %ld, id = 0x%x, seq = %d, timeout = %d ms)\n", testIP, sizeof(packet.icmp_all.icmp_data),pid,icmp_req,timeout);

				//fill ip and icmp header
				fill_icmphdr(&packet.icmp_all,data);
				fill_iphdr(&packet.ip_hdr, testIP,IP,sizeof(packet));
				unsigned long timeUsec;
				unsigned long timeSec;
				//set timer
				gettimeofday(&start, NULL);
				if(sendto(sockfd, &packet, sizeof(packet), 0, &dst, sizeof(dst)) < 0)
				{
					perror("sendto");
					exit(1);
				}

				if((sockfd = socket(AF_INET, SOCK_RAW , IPPROTO_ICMP)) < 0)
				{
					perror("socket");
					exit(1);
				}
				middle.tv_sec = timeout/1000;
				bzero(&dst,sizeof(dst));
				//int status=1;
				while(1){
					if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&middle,sizeof(struct timeval)) == -1){
						
			    	}
			    		if(recvfrom(sockfd, &packet, sizeof(packet), 0,  NULL, NULL) < 0){
				            printf("Destination Unreachable\n\n");
				            break;
						}
						gettimeofday(&stop, NULL);
						timeSec = stop.tv_sec-start.tv_sec;
						timeUsec =(stop.tv_usec-start.tv_usec);
						if(ntohs(packet.icmp_all.icmp_type) == ICMP_ECHOREPLY )
			        	{
			            	printf("Reply from : %s , time : %ld.%04ld ms\n\n",testIP,timeSec,timeUsec);
			            	break;
			        	}	
				}
				icmp_req++;
				
			}
		}
		else{
				print_usage();
				exit(1);
		}
	}
	else{
		print_usage();
		exit(1);
	}

	return 0;
}