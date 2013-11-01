/* netscout-common.c
 * collection of common functions for the library
 *
 * author: mikeyhc <mikeyhc@atmosia.net>
 */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct ifaddrs *ifaddr = NULL;		/* store for single fetch */

/* load_ifaddrs: this will check if we have fetched ifaddrs yet, and if not will
 * fetch them */
void
load_ifaddrs(void){
	if(ifaddr!=NULL) return;
	if(getifaddrs(&ifaddr)==-1){
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}
} /* end: load_ifaddrs */

/* map_local_network: will get all the interfaces on this machine, then get the
 * network and netmask for each interface, using that it will attempt to ping 
 * every possible host in each network and print the results.
 */
void
map_local_network(void){
	struct ifaddrs *ifa;
	int family, s, i, j;
	char host[NI_MAXHOST], mask[NI_MAXHOST], network[NI_MAXHOST], 
			 broadcast[NI_MAXHOST];
	struct sockaddr net, broad;
	
	load_ifaddrs();
	for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next){
		if(ifa->ifa_addr==NULL) continue;
		family = ifa->ifa_addr->sa_family;
		if(family != AF_INET && family != AF_INET6) continue;

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
		printf("\taddress: <%s>\n", host);
	
		s = getnameinfo(ifa->ifa_netmask,
				(family == AF_INET) ? sizeof(struct sockaddr_in) :
															sizeof(struct sockaddr_in6),
				mask, NI_MAXHOST, NULL, 0 , NI_NUMERICHOST);
		if(s!=0){
			printf("getnameinfo() failed: %s\n", gai_strerror(s));
			exit(EXIT_FAILURE);
		}
		printf("\tnetmask: <%s>\n", mask);

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
		printf("\tnetwork: <%s>\n", network);

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

	}
} /* end: map_local_network */
