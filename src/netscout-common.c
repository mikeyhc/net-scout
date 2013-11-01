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
#include <string.h>

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
			s = getnameinfo(&sa_temp,
					(family == AF_INET) ? sizeof(struct sockaddr_in) :
																sizeof(struct sockaddr_in6),
					host_temp, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if(s!=0){
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				exit(EXIT_FAILURE);
			}
			printf("testing %s: \n", host_temp);
			increase_sockaddr(&sa_temp, family, 1);
		}
	}
} /* end: map_local_network */
