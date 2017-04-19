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



int print_radiotap_header_old(const u_char *Buffer, int Size){
	// This struct is the RadioTap header: https://radiotap.org
	struct radiotap_header{ // RadioTap is the standard for 802.11 reception/transmission/injection
		uint8_t it_rev; // Revision: Version of RadioTap
		uint8_t it_pad; // Padding: 0 - Aligns the fields onto natural word boundaries
		uint16_t it_len;// Length: 26 - entire length of RadioTap header
	};
	// These are placeholders for offset values:
	const u_char *bssid; // a place to put our BSSID \ these are bytes
	const u_char *essid; // a place to put our ESSID / from the packet
	const u_char *essidLen;
	const u_char *channel; // the frequency (in Mhz) of the AP Radio
	const u_char *rssi; // received signal strength
	/*
	 * Determining the Security protocol: Auth Key Mgmt: 00-0f-ac-01 == EAP, 00-0f-ac-02 == PSK
	 * Pairwise Cipher Suite, PCS
	 * Auth Key MGMT, AKM
	 *
	 * These are "tagged params" and need to be parsed programmatically
	 *
	 * WPA2/PSK/CCMP = (AKM:00-0f-ac-02,PCS:00-0f-ac-04) tag-type:0x30
	 * WEP 40 = PCS:00-0f-ac-01
	 * WEP 104 = PCS:00-0f-ac-05
	 * TKIP = PCS:00-0f-ac-02
	 * Microsoft Suite TKIP = PCS:00-50-f2-02, tag-type: 0xdd
	 * WPA2 Enterprise = (AKM:00-0f-ac-04,PCS:00-0f-ac-04) tag-type: 0x30
	 *
	 */
	int offset = 0;
	struct radiotap_header *rtaphdr;
	rtaphdr = (struct radiotap_header *) Buffer;
	offset = rtaphdr->it_len; // 26 bytes on my machine
	bssid = Buffer + 42; // store the BSSID/AP MAC addr, 36 byte offset is transmitter address
	rssi = Buffer + 22; // this is hex and this value is subtracted from 256 to get -X dbm.
	signed int rssiDbm = rssi[0] - 256;
	channel = Buffer + 18; // channel in little endian format (2 bytes)
	int channelFreq = channel[1] * 256 + channel[0]; // a little bit of math, remember little endian
	// 87 byte offset contains the "channel number" as per 802.11, e.g. 2412 = "channel 11"
	
	fprintf(stdout,"RSSI: %d dBm\n",rssiDbm);
	fprintf(stdout,"AP Frequency: %iMhz\n",channelFreq);
	fprintf(stdout,"BSSID string: %02X:%02X:%02X:%02X:%02X:%02X\n",bssid[0],bssid[1],bssid[2],bssid[3],bssid[4],bssid[5]);

	return offset;
}


int print_radiotap_header(const u_char *Buffer, int Size){
	struct ieee80211_radiotap_iterator iter;
	void *data = (void*)Buffer;
	int err;
	int offset;
	
	err = ieee80211_radiotap_iterator_init(&iter, data, Size, NULL);
	if (err) {
		printf("malformed radiotap header (init returns %d)\n", err);
		return -3;
	}
	offset = iter._max_length;
	while (!(err = ieee80211_radiotap_iterator_next(&iter))) {
		if (iter.this_arg_index == IEEE80211_RADIOTAP_DBM_ANTSIGNAL) {
			//printf("RSSI = %i", (int)iter.this_arg[0] - 256);
            RSSI=(int)iter.this_arg[0] - 256;
            //printf("RSSI=%i\n",RSSI);
			//printf("\n");
		}
        if (iter.this_arg_index == IEEE80211_RADIOTAP_CHANNEL) {
            //printf("CH = %i", (int)iter.this_arg[1]*256+iter.this_arg[0]);

            CH=le16toh(*(uint16_t*)iter.this_arg);
            //printf("CH %i",CH);
            //printf("\n");
        }
	}

	if (err != -ENOENT) {
		printf("malformed radiotap data\n");
		return -3;
	}

	return offset;
}

void print_ethernet_header(const u_char *Buffer, int Size){
	const u_char *source_mac_addr;
	const u_char *destination_mac_addr;
	destination_mac_addr = Buffer + 4;
	source_mac_addr = Buffer + 10;
	//fprintf(stdout,"Source MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",source_mac_addr[0],source_mac_addr[1],source_mac_addr[2],source_mac_addr[3],source_mac_addr[4],source_mac_addr[5]);
	//fprintf(stdout,"Destination MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",destination_mac_addr[0],destination_mac_addr[1],destination_mac_addr[2],destination_mac_addr[3],destination_mac_addr[4],destination_mac_addr[5]);
    //SA=source_mac_addr[0],source_mac_addr[1],source_mac_addr[2],source_mac_addr[3],source_mac_addr[4],source_mac_addr[5];
    for(int i = 0; i < 6; ++i)
    {
        sprintf(SA + 2*i, "%02X", source_mac_addr[i]);
        sprintf(DA + 2*i, "%02X", destination_mac_addr[i]);

    }
    //printf("SA  =%s\n",SA);
    //printf("SA  =%s",DA);

}
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
void my_callback(u_char *args, const struct pcap_pkthdr* header, const u_char* packet)
{
	int offset;
	int size = header->len;
	offset = print_radiotap_header(packet, size);
	if(offset > 0)
		print_ethernet_header(packet + offset, size);
	global++;
	fprintf(stdout,"/*-----------------------------------Pkt #%i-----------------------------------*/\n", global);
	fflush(stdout);
    cjson_struts_init();


}
int get_local_mac(char *eth)
 {
     int sock;
     int res;
     struct ifreq ifr;

     sock = socket(AF_INET, SOCK_STREAM, 0);
     strcpy(ifr.ifr_name,eth);
     res = ioctl(sock, SIOCGIFADDR, &ifr);

     //printf("IP: %s\n",inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));

     //strcpy(ifr.ifr_name, "ens331asdfas");
     res = ioctl(sock, SIOCGIFHWADDR, &ifr);

     int i;
     //char mac[32];
     for(i = 0; i < 6; ++i)
     {
         sprintf(AP_MAC + 2*i, "%02x", (unsigned char)ifr.ifr_hwaddr.sa_data[i]);
     }
     //printf("MAC: %s\n",mac);

     return 0;
 }


int main(int argc,char **argv)
{

	init_get_config_parameters(); /*init config file read*/

	get_local_mac(loc_Adapter);		/*local adapter card true mac*/

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
	/* Check if the network interface provides the radiotap header */
	if (pcap_datalink(descr) != DLT_IEEE802_11_RADIO) {
		fprintf(stderr, "Device %s doesn't provide 802.11 radiotap header - not supported\n", dev);
		return(2);
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
	/* loop for callback function */
	pcap_loop(descr, -1, my_callback, NULL);
	return 0;
}
