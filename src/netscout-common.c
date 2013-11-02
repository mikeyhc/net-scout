/* netscout-common.c
 * collection of common functions for the library
 *
 * author: mikeyhc <mikeyhc@atmosia.net>
 */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 400

static struct ifaddrs *ifaddr = NULL;		/* store for single fetch */

/* load_ifaddrs: this will check if we have fetched ifaddrs yet, and if not will
 * fetch them */
static void
load_ifaddrs(void){
	if(ifaddr!=NULL) return;
	if(getifaddrs(&ifaddr)==-1){
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}
} /* end: load_ifaddrs */

/* sockaddrcpy: copies relevant data from one sockaddr to another
 *
 * param dest: the structure to copy into
 * param src: the structure to copy from
 * param family: either AF_INET or AF_INET6
 */
static void
sockaddrcpy(struct sockaddr *dest, struct sockaddr *src, int family){
	int i;

	if(family == AF_INET)
		for(i=2; i<6; i++)
			dest->sa_data[i] = src->sa_data[i];
	else if(family == AF_INET6) { }
	dest->sa_family = family;
}/* end: sockaddrcpy */

/* compare_sockaddr: compares two sockaddr and returns and int representing
 * which is greater 
 * 
 * param a: the left hand side of the compare
 * param b: the right hand side of the compare
 * param family: either AF_INET or AF_INET6
 * returns: -1 if a>b, 0 if a==b, 1 if a<b
 */
static int
compare_sockaddr(struct sockaddr *a, struct sockaddr *b, int family){
	int i;

	if(family == AF_INET)
		for(i=2; i<6; i++){
			if((a->sa_data[i] & 0xFF) > (b->sa_data[i] & 0xFF)) return -1;
			else if((a->sa_data[i] & 0xFF) < (b->sa_data[i] & 0xFF)) return 1;
		}
	else if(family == AF_INET6){ }

	return 0;
}/* end: compare_sockaddr */

/* increase_sockaddr: increases a sockaddr by a given amount
 *
 * param a: sockaddr to increase
 * param family: AF_INET or AF_INET6
 * param increase: the amount to increase by
 */
static void
increase_sockaddr(struct sockaddr *a, int family, int increase){
		int i, temp;

		if(family == AF_INET){
			for(i=5; i>1; i--){
				temp = (a->sa_data[i] & 0xFF) + increase;
				a->sa_data[i] = temp % 0x100;
				increase = temp / 0x100;
				if(!increase) break;
			}
		}else if(family == AF_INET6){

		}
}/* end: increase_sockaddr */

/* testping: sends a ping from a given source address to a given host address
 * and will prrint a message if there is a reply
 *
 * param src: the address to send from
 * param dst: the address to send to
 */
static void
testping(struct sockaddr *src, struct sockaddr *dst){
	int s, on, dstlen;
	struct ip *ip;
	struct icmphdr *icmp;
	char buf[BUFFER_SIZE];

	ip = (struct ip*)buf;
	icmp = (struct icmphdr*)(ip + 1);
	memset(buf, 0, BUFFER_SIZE);

	if((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) , 0){
		perror("socket() error");
		exit(1);
	}

	on = 1;
	if(setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0){
		perror("setsockopt() for BROADCAST error");
		exit(EXIT_FAILURE);
	}

	if(setsockopt(s, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0){
		perror("setsockopt() for IP_HDRINCL error");
		exit(EXIT_FAILURE);
	}

	ip->ip_v = 4;
	ip->ip_hl = sizeof*ip >> 2;
	ip->ip_tos = 0;
	ip->ip_len = htons(sizeof(buf));
	ip->ip_id = htons(4321);
	ip->ip_off = htons(0);
	ip->ip_ttl = 255;
	ip->ip_p = 1;
	ip->ip_sum = 0; /* let the kernel do its thang */
	
	icmp->type = ICMP_ECHO;
	icmp->code = 0;
	icmp->checksum = htons(~(ICMP_ECHO << 8));

	ip->ip_src.s_addr = 0x00000000;
	ip->ip_src.s_addr |= (src->sa_data[5] & 0xFF) << 24;
	ip->ip_src.s_addr |= (src->sa_data[4] & 0xFF) << 16;
	ip->ip_src.s_addr |= (src->sa_data[3] & 0xFF) << 8;
	ip->ip_src.s_addr |= (src->sa_data[2] & 0xFF);

	ip->ip_dst.s_addr = 0x00000000;
	//ip->ip_dst.s_addr |= (dst->sa_data[5] & 0xFF) << 24;
	ip->ip_dst.s_addr |= (2 & 0xFF) << 24;
	ip->ip_dst.s_addr |= (dst->sa_data[4] & 0xFF) << 16;
	ip->ip_dst.s_addr |= (dst->sa_data[3] & 0xFF) << 8;
	ip->ip_dst.s_addr |= (dst->sa_data[2] & 0xFF);

	printf("dst: %s\n", inet_ntoa(ip->ip_dst));
	dstlen = sizeof(*dst);
	if(sendto(s, buf, sizeof(buf), 0, dst, dstlen) < 0){
		perror("error with sendto()");
		exit(EXIT_FAILURE);
	}

	if(recvfrom(s, buf, sizeof(buf), 0, dst, &dstlen) < 0){
		perror("error with recvfrom()");
		exit(EXIT_FAILURE);
	}

	close(s);
}/* end: testping */

/* map_local_network: will get all the interfaces on this machine, then get the
 * network and netmask for each interface, using that it will attempt to ping 
 * every possible host in each network and print the results.
 */
void
map_local_network(void){
	struct ifaddrs *ifa;
	int family, s, i, j;
	char host[NI_MAXHOST], mask[NI_MAXHOST], network[NI_MAXHOST], 
			 broadcast[NI_MAXHOST], host_temp[NI_MAXHOST];
	struct sockaddr net, broad, sa_temp;
	
	load_ifaddrs();
	for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
		if(ifa->ifa_addr==NULL) continue;
		family = ifa->ifa_addr->sa_family;
		if(family != AF_INET && family != AF_INET6) continue;
                if(!strcmp(ifa->ifa_name, "lo")) continue; /* skip loopback */

		printf("\nusing:\n%s address family: %d%s\n", 
				ifa->ifa_name, family,
				(family == AF_INET)		? " (AF_INET)" :
				(family == AF_INET6)	? " (AF_INET6)" : " (UNKNOWN)");
		s = getnameinfo(ifa->ifa_addr,
				(family == AF_INET) ? sizeof(struct sockaddr_in) :
															sizeof(struct sockaddr_in6),
				host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
		if(s!=0){
			printf("getnameinfo() failed: %s\n", gai_strerror(s));
			exit(EXIT_FAILURE);
		}
		printf("\taddress:   <%s>\n", host);
	
		s = getnameinfo(ifa->ifa_netmask,
				(family == AF_INET) ? sizeof(struct sockaddr_in) :
															sizeof(struct sockaddr_in6),
				mask, NI_MAXHOST, NULL, 0 , NI_NUMERICHOST);
		if(s!=0){
			printf("getnameinfo() failed: %s\n", gai_strerror(s));
			exit(EXIT_FAILURE);
		}
		printf("\tnetmask:   <%s>\n", mask);

		for(i=0; i<(family == AF_INET ? sizeof(struct sockaddr_in) :
				sizeof(struct sockaddr_in6)); i++)
			net.sa_data[i] = ifa->ifa_addr->sa_data[i] & ifa->ifa_netmask->sa_data[i];
		net.sa_family = family;
		s = getnameinfo(&net,
				(family == AF_INET) ? sizeof(struct sockaddr_in) :
															sizeof(struct sockaddr_in6),
				network, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
		if(s!=0){
			printf("getnameinfo() failed: %s\n", gai_strerror(s));
			exit(EXIT_FAILURE);
		}
		printf("\tnetwork:   <%s>\n", network);

		for(i=0; i<(family == AF_INET ? sizeof(struct sockaddr_in) :
				sizeof(struct sockaddr_in6)); i++)
			broad.sa_data[i] = (ifa->ifa_addr->sa_data[i] &	ifa->ifa_netmask->sa_data[i]) |
					~ifa->ifa_netmask->sa_data[i];
		broad.sa_family = family;
		s = getnameinfo(&broad,
				(family == AF_INET) ? sizeof(struct sockaddr_in) :
															sizeof(struct sockaddr_in6),
				broadcast, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
		if(s!=0){
			printf("getnameinfo() failed: %s\n", gai_strerror(s));
			exit(EXIT_FAILURE);
		}
		printf("\tbroadcast: <%s>\n", broadcast);

		sockaddrcpy(&sa_temp, &net, family);
		increase_sockaddr(&sa_temp, family, 1);
		while(compare_sockaddr(&broad, &sa_temp, family)<0){
			testping(ifa->ifa_addr, &sa_temp);
			increase_sockaddr(&sa_temp, family, 1);
		}
	}
} /* end: map_local_network */
