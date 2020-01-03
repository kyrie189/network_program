#include <stdio.h>  
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <pcap.h>  
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#define DEVICE            "enp0s3"
#define URL_MAX_LEN        2048
#define MAX_HOST_LEN    1024
#define MAX_GET_LEN        2048

#define get_u_int8_t(X,O)  (*(uint8_t *)(((uint8_t *)X) + O))
#define get_u_int16_t(X,O)  (*(uint16_t *)(((uint8_t *)X) + O))
#define get_u_int32_t(X,O)  (*(uint32_t *)(((uint8_t *)X) + O))
#define get_u_int64_t(X,O)  (*(uint64_t *)(((uint8_t *)X) + O))

typedef struct sdaddr *taddr;
struct sdaddr
{
    char saddr[50];
    char daddr[50];
    int num;
};
taddr addr1;
taddr addr5;
int IP_count;
void compare_addr(taddr addr1, int IP_count,taddr addr5 ,int *num)
{
    int i = 0;
    int j;
    (*num) = 0;
    for (i = 0;i < IP_count;i++)
    {
        char *ptr = addr1[i].saddr;
        char *ptr1 = addr1[i].daddr;
       if (i == 0)
       {
           strcpy(addr5[(*num)].saddr,addr1[i].saddr);
           strcpy(addr5[(*num)].daddr,addr1[i].daddr);
           addr5[i].num++;
           (*num)++; 
       }
       else 
       {
           for (j = 0;j < (*num);j++)
           {
               char *qtr = addr5[j].saddr;
               char *qtr1 = addr5[j].daddr;
                if (strcmp(ptr,qtr) == 0 && strcmp(ptr1,qtr1) == 0)
                {
                    addr5[j].num++;
                    break;
                }
           }
            if (j != (*num))
            {
                continue;
            }
            else 
            {
                strcpy(addr5[(*num)].saddr,addr1[i].saddr);
                strcpy(addr5[(*num)].daddr,addr1[i].daddr);
                addr5[(*num)].num++;
                (*num)++;
            }
       }

    }
}


 /*Display Ethernet Header*/
 void show_ethhdr(struct ethhdr *eth)
 {
    printf("----------------eth---------------------\n");
    printf("destination eth addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
         eth->h_dest[0], eth->h_dest[1],
         eth->h_dest[2], eth->h_dest[3],
         eth->h_dest[4], eth->h_dest[5]);
    printf("source eth addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
         eth->h_source[0], eth->h_source[1],
         eth->h_source[2], eth->h_source[3],
         eth->h_source[4], eth->h_source[5]);
     printf("protocol is: %04x\n", ntohs(eth->h_proto));
 }

/*Display IP Header*/
void show_iphdr(struct iphdr *ip)
{
    
    struct in_addr addr;
    struct in_addr addr2;
    printf("----------------ip----------------------\n");
    printf("version: %d\n", ip->version);
    printf("head len: %d\n", ip->ihl * 4);
    printf("total len: %d\n", ntohs(ip->tot_len));
    printf("ttl: %d\n", ip->ttl);
    printf("protocol: %d\n", ip->protocol);
    printf("check: %x\n", ip->check);
    addr.s_addr = ip->saddr;
    printf("saddr: %s\n", inet_ntoa(addr));
    addr2.s_addr = ip->daddr;
    printf("daddr: %s\n", inet_ntoa(addr2));
    strcpy( addr1[IP_count].saddr, inet_ntoa(addr));
    strcpy( addr1[IP_count].daddr, inet_ntoa(addr2));
    IP_count++;
 }
   
 /*Display TCP Header*/
 void show_tcphdr(struct tcphdr *tcp)
{
    printf("----------------tcp---------------------\n");
   printf("tcp len: %ld\n", sizeof(struct tcphdr));
    printf("tcp->doff: %d\n", tcp->doff * 4);
    printf("source port: %d\n", ntohs(tcp->source));
    printf("dest port: %d\n", ntohs(tcp->dest));
    printf("sequence number: %d\n", ntohs(tcp->seq));
   printf("ack sequence: %d\n", ntohs(tcp->ack_seq));
}
 
 int parse_http_head(const u_char *payload, int payload_len, char *url)
{
    int line_len, offset;
    int ustrlen;
   int hstrlen; //"host: " 
   int hostlen;
  int getlen; 
    char host[MAX_HOST_LEN];
    char get[MAX_GET_LEN]; 
   int a, b;
      
    /*filter get packet*/
    if(memcmp(payload, "GET ", 4)) 
    {
       return 0;
    }

    for(a = 0, b = 0; a < payload_len - 1; a++) {
       if (get_u_int16_t(payload, a) == ntohs(0x0d0a)) {
          line_len = (u_int16_t)(((unsigned long) &payload[a]) - ((unsigned long)&payload[b]));
    
            if (line_len >= (9 + 4)
                 && memcmp(&payload[line_len - 9], " HTTP/1.", 8) == 0) {
                memcpy(get, payload + 4, line_len - 13); //"GET  HTTP/1.x" 13bit
               getlen = line_len - 13;
             }   
             /*get url host of pcaket*/
             if (line_len > 6 
                && memcmp(&payload[b], "Host:", 5) == 0) {
               if(*(payload + b + 5) == ' ') {
                    hstrlen = b + 6;
                } else {
                   hstrlen = b + 5;
                }
                hostlen = a - hstrlen;   
                memcpy(host, payload + hstrlen, (a - hstrlen));
             }   
            b = a + 2;
        }   
   }
    offset =  7;
     memcpy(url, "http://", offset);
    memcpy(url + offset, host, hostlen);
     offset += hostlen;
   memcpy(url + offset, get, getlen);

   return strlen(url);
}

void packet_http_handle(const u_char *tcp_payload, int payload_len)
{    
   int url_len;
    char url[URL_MAX_LEN];
    
   url_len = parse_http_head(tcp_payload, payload_len, url);
   if (url_len <= 7) 
   {
       return;    
   }
    printf("----------------HTTP---------------------\n");
     printf("url_len: %d\n", url_len);
    printf("url: %s\n", url);
 }

int prase_packet(const u_char *buf,  int caplen)
{
    uint16_t e_type;
    uint32_t offset;
    int payload_len;
    const u_char *tcp_payload;
    
   /* ether header */
    struct ethhdr *eth = NULL;
    eth = (struct ethhdr *)buf;
    e_type = ntohs(eth->h_proto);
    offset = sizeof(struct ethhdr);
    show_ethhdr(eth);
 
   /*vlan 802.1q*/    
   while(e_type == ETH_P_8021Q)     
   {
        e_type = (buf[offset+2] << 8) + buf[offset+3];
        offset += 4;
   }  
   if (e_type != ETH_P_IP) 
   {
       printf("/********************\nIt is not an IP packet\n********************************/\n");
        return -1;
   }   
  
     /* ip header */    
    struct iphdr *ip = (struct iphdr *)(buf + offset);
    e_type = ntohs(ip->protocol);
    offset += sizeof(struct iphdr);
    show_iphdr(ip);
     
    if(ip->protocol != IPPROTO_TCP && ip->protocol != IPPROTO_UDP) 
    {
       return -1;
    }
 
     /*tcp header*/
    struct tcphdr *tcp = (struct tcphdr *)(buf + offset);
    offset += (tcp->doff << 2);
    payload_len = caplen - offset;
    tcp_payload = (buf + offset);
    show_tcphdr(tcp);

   /*prase http header*/
    packet_http_handle(tcp_payload, payload_len);
    
    return 0;
}

void get_packet(u_char *user, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
    static int count = 0;
    printf("\n----------------------------------------\n");
    printf("\t\tpacket %d\n", count);
    printf("----------------------------------------\n");
    printf("Packet id: %d\n", count);
    printf("Packet length: %d\n", pkthdr->len);  
    printf("Number of bytes: %d\n", pkthdr->caplen);  
    printf("Recieved time: %s\n", ctime((const time_t *)&pkthdr->ts.tv_sec));
    prase_packet(packet, pkthdr->len);
    count++;
}

int main(int argc ,char**argv)  
{  
    int i;
    int num = 0;
    addr1 = (taddr)malloc(sizeof(struct sdaddr)*100);
    addr5 = (taddr)malloc(sizeof(struct sdaddr)*100);
    for (i = 0;i < 100;i++)
    {
        addr1[i].num = 0;
        addr5[i].num = 0;
    }
    IP_count = 0;
    char error_buffer[PCAP_ERRBUF_SIZE]; /*error Buff*/
    struct pcap_pkthdr packet;  /*The header that pcap gives us*/
    pcap_t *dev; /*network interface*/
    bpf_u_int32 netp, maskp; 
    char *net, *mask;
    struct in_addr addr;
    int ret;
    pcap_t *handle;
    /*look up device network addr and mask*/
    if (argc != 3)
    {
        printf("please -r pcap_file\n");
        exit(1);
    }    
    char *name = argv[2];
    handle = pcap_open_offline(name,error_buffer);
    
    /*process packets from a live capture or savefile*/
    pcap_loop(handle, 0, get_packet, NULL);
    pcap_close(handle);
    compare_addr(addr1, IP_count,addr5,&num);
    for (i = 0;i < num;i++) 
    {
        printf("%s -> %s  %d\n",addr5[i].saddr,addr5[i].daddr,addr5[i].num);
    }
    printf("ip count %d\n",num);

    /*printf("compare\n");
    if (strcmp(addr1[40].saddr,addr1[41].saddr) == 0 && strcmp(addr1[40].daddr,addr1[41].daddr) == 0)
    {
        printf("11111\n");
    }*/
     return 0; 
} 

