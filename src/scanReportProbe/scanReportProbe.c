#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <netinet/if_ether.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <curl/curl.h>
#include "radiotap_iter.h"
#include "handler_config.h"  //read /etc/my_app_name/config_name.conf
#include "cJSON.h"
int global = 0;
int CH;
int RSSI;
char SA[24];
char DA[24];
char AP_MAC[24];

#define SNAP_LEN 1518       // 以太网帧最大长度
#define SIZE_ETHERNET 14   // 以太网包头长度 mac 6*2, type: 2
#define ETHER_ADDR_LEN  6  // mac地址长度

//以太网卡头信息 MAC 和type 共计14
struct packet_ethernet {
    u_char  ether_dhost[ETHER_ADDR_LEN];    /* destination host address */
    u_char  ether_shost[ETHER_ADDR_LEN];    /* source host address */
    u_short ether_type;                     /* IP? ARP? RARP? etc */
};

/* IP header */
struct packet_ip {
    u_char  ip_vhl;                 /* version << 4 | header length >> 2 */
    u_char  ip_tos;                 /* type of service */
    u_short ip_len;                 /* total length */
    u_short ip_id;                  /* identification */
    u_short ip_off;                 /* fragment offset field */
    #define IP_RF 0x8000            /* reserved fragment flag */
    #define IP_DF 0x4000            /* dont fragment flag */
    #define IP_MF 0x2000            /* more fragments flag */
    #define IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
    u_char  ip_ttl;                 /* time to live */
    u_char  ip_p;                   /* protocol */
    u_short ip_sum;                 /* checksum */
    struct  in_addr ip_src,ip_dst;  /* source and dest address */
    //struct in_addr ip_src;
    //struct in_addr ip_dst;              /* source and dest address */
};

#define IP_HL(ip)               (((ip)->ip_vhl) & 0x0f)
#define IP_V(ip)                (((ip)->ip_vhl) >> 4)
 
/* TCP header */
typedef u_int tcp_seq;

struct packet_tcp {
    u_short th_sport;               /* source port */
    u_short th_dport;               /* destination port */
    tcp_seq th_seq;                 /* sequence number */
    tcp_seq th_ack;                 /* acknowledgement number */
    u_char  th_offx2;               /* data offset, rsvd */
    #define TH_OFF(th)      (((th)->th_offx2 & 0xf0) >> 4)
    u_char  th_flags;
    #define TH_FIN  0x01
    #define TH_SYN  0x02
    #define TH_RST  0x04
    #define TH_PUSH 0x08
    #define TH_ACK  0x10
    #define TH_URG  0x20
    #define TH_ECE  0x40
    #define TH_CWR  0x80
    #define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
    u_short th_win;                 /* window */
    u_short th_sum;                 /* checksum */
    u_short th_urp;                 /* urgent pointer */
};

int cjson_struts_init(){
    cJSON * pJsonRoot = NULL;


    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        //error happend here
        return 2;
    }
    //json struct for cjson
    cJSON_AddStringToObject(pJsonRoot, "AP_MAC", AP_MAC);
    cJSON_AddStringToObject(pJsonRoot, "DA_MAC", SA);
    cJSON_AddStringToObject(pJsonRoot, "BSSID_MAC", DA);
    cJSON_AddNumberToObject(pJsonRoot, "Db", RSSI);
    cJSON_AddNumberToObject(pJsonRoot, "CH", CH);
    //cJSON_AddBoolToObject(pJsonRoot, "bool", 1);
    char * p = cJSON_Print(pJsonRoot);

    if(NULL == p)
    {
        //convert json list to string faild, exit
        //because sub json pSubJson han been add to pJsonRoot, so just delete pJsonRoot, if you also delete pSubJson, it will coredump, and error is : double free
        cJSON_Delete(pJsonRoot);
        return 1;
    }
    printf("%s\n", p);
    return 0;
}

void loop_callback(u_char *args, const struct pcap_pkthdr *header, const u_char *packet) {
    static int count = 0;                   // 包计数器
    const struct packet_ethernet *ethernet;  /* The ethernet header [1] */
    const struct packet_ip *ip;              /* The IP header */
    const struct packet_tcp *tcp;            /* The TCP header */
    const char *payload;                     /* Packet payload */
 
    int size_ip;
    int size_tcp;
    int size_payload;
 
    count++;
 
    /* 以太网头 */
    ethernet = (struct packet_ethernet*)(packet);
 
    /* IP头 */
    ip = (struct packet_ip*)(packet + SIZE_ETHERNET);
    size_ip = IP_HL(ip)*4;
    if (size_ip < 20) {
        //printf("无效的IP头长度: %u bytes\n", size_ip);
        return;
    }
 
    if ( ip->ip_p != IPPROTO_TCP ){ // TCP,UDP,ICMP,IP
    return;
    }
 
    /* TCP头 */
    tcp = (struct packet_tcp*)(packet + SIZE_ETHERNET + size_ip);
    size_tcp = TH_OFF(tcp)*4;
    if (size_tcp < 20) {
        //printf("无效的TCP头长度: %u bytes\n", size_tcp);
        return;
    }
 
    int sport =  ntohs(tcp->th_sport);
    int dport =  ntohs(tcp->th_dport);
    printf("%s:%d ", inet_ntoa(ip->ip_src), sport);
    printf("%s:%d \n", inet_ntoa(ip->ip_dst), dport);
 
    //内容
    payload = (u_char *)(packet + SIZE_ETHERNET + size_ip + size_tcp);
 
    //内容长度 
    size_payload = ntohs(ip->ip_len) - (size_ip + size_tcp);
 
    if (size_payload > 0) {
    	//printf("%d %d %d bytes\n", ntohs(tcp->th_seq), ntohs(tcp->th_ack) );
    	write(payload, size_payload);
    } else {
   	  //printf("%d %d %d \n", ntohs(tcp->th_seq), ntohs(tcp->th_ack));
    }
}




int main(int argc,char **argv)
{

	init_get_config_parameters(); /*init config file read*/


	printf("Start\n");
	char *dev;
	char errbuf[PCAP_ERRBUF_SIZE];
	pcap_t* descr;
	char *filter_exp = PKG_TYPE_TMP;	/* The filter expression */
	const u_char *packet;
	struct pcap_pkthdr hdr;
	struct ether_header *eptr;    /* net/ethernet.h */
	struct bpf_program fp;        /* hold compiled program */
	bpf_u_int32 maskp;            /* subnet mask */
	bpf_u_int32 netp;             /* ip */

	/* Now get a device */
	//dev = pcap_lookupdev(errbuf);
	dev = INTERFACE_TMP;
	printf("Interface: %s\n", dev); 

	if(dev == NULL) {
		fprintf(stderr, "%s\n", errbuf);
		exit(1);
	}
	/* Get the network address and mask */
	pcap_lookupnet(dev, &netp, &maskp, errbuf);

	/* open device for reading in promiscuous mode */
	descr = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
	if(descr == NULL) {
		printf("pcap_open_live(): %s\n", errbuf);
		exit(1);
	}

	/* Compile and apply the filter */
	if (pcap_compile(descr, &fp, filter_exp, 0, netp) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(descr));
		return(2);
	}
	if (pcap_setfilter(descr, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(descr));
		return(2);
	}
	//close config file
	config_destroy_init();
	printf("config destroy");
	/* loop for callback function */
	pcap_loop(descr, -1, loop_callback, NULL);
	return 0;
}
