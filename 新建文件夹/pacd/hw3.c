#include <stdio.h>
#include <pcap.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>



int IP_packet;
/* Finds the payload of a TCP/IP packet */
void my_packet_handler(u_char *args,const struct pcap_pkthdr *header,const u_char *packet)  //packet
{
    unsigned char *p_mac_string = NULL;			// 保存mac的地址，临时变量

    /* First, lets make sure we have an IP packet */
    struct ether_header *eth_header;        //ethernet header
    eth_header = (struct ether_header *) packet;

    //capture sourse MAC and dest MAC
    p_mac_string = (unsigned char *)eth_header->ether_shost;//获取源mac
    printf("Mac Source Address is %02x:%02x:%02x:%02x:%02x:%02x\n",*(p_mac_string+0),*(p_mac_string+1),*(p_mac_string+2),*(p_mac_string+3),*(p_mac_string+4),*(p_mac_string+5));
    p_mac_string = (unsigned char *)eth_header->ether_dhost;//获取目的mac
    printf("Mac Destination Address is %02x:%02x:%02x:%02x:%02x:%02x\n",*(p_mac_string+0),*(p_mac_string+1),*(p_mac_string+2),*(p_mac_string+3),*(p_mac_string+4),*(p_mac_string+5));
    printf("\n\n");
    printf("Total packet available: %d bytes\n", header->caplen);
    printf("Expected packet size: %d bytes\n\n\n", header->len);
    // IP AND TCP OR UDP
    if (ntohs(eth_header->ether_type) == ETHERTYPE_IP)      //IP packet
    {
        IP_packet++;
        //display the 還可以顯示來源IP位址與目的IP位址

        /* Pointers to start point of various headers */
        const u_char *ip_header;
        const u_char *tcp_header;
        const u_char *payload;

        /* Header lengths in bytes */
        int ethernet_header_length = 14; /* Doesn't change */
        int ip_header_length;
        int tcp_header_length;
        int payload_length;

        /* Find start of IP header */
        ip_header = packet + ethernet_header_length;
        printf("%d\n",ip_header);

        /* The second-half of the first byte in ip_header
        contains the IP header length (IHL). */
        ip_header_length = ((*ip_header) & 0x0F);
         /* The IHL is number of 32-bit segments. Multiply
        by four to get a byte count for pointer arithmetic */
        ip_header_length = ip_header_length * 4;
        printf("IP header length (IHL) in bytes: %d\n", ip_header_length);
        u_char protocol = *(ip_header + 9);
        if (protocol == IPPROTO_TCP || protocol == IPPROTO_UDP)        //TCP
        {
            printf("\n\nTCP packet");
             /* Add the ethernet and ip header length to the start of the packet
                to find the beginning of the TCP header */
            tcp_header = packet + ethernet_header_length + ip_header_length;
            /* TCP header length is stored in the first half 
                of the 12th byte in the TCP header. Because we only want
                the value of the top half of the byte, we have to shift it
                down to the bottom half otherwise it is using the most 
                significant bits instead of the least significant bits */
            tcp_header_length = ((*(tcp_header + 12)) & 0xF0) >> 4;  
            tcp_header_length = tcp_header_length * 4;
            printf("TCP header length in bytes: %d\n", tcp_header_length);
            /* Add up all the header sizes to find the payload offset */
            int total_headers_size = ethernet_header_length+ip_header_length+tcp_header_length;
            printf("Size of all headers combined: %d bytes\n", total_headers_size);
            payload_length = header->caplen - (ethernet_header_length + ip_header_length + tcp_header_length);
            printf("Payload size: %d bytes\n", payload_length);
            payload = packet + total_headers_size;
            printf("Memory address where payload begins: %p\n\n", payload);
            printf("\n\n");
            if (protocol == IPPROTO_UDP)
            {
                printf("It is an UDP packet\n");
            }
            else 
            {
                printf("It is an TCP packet\n");
            }
            //display the TCP/UDP的port號碼

            
            //display the 每對(來源IP,目的IP)的封包 and number


        }
        else
        {
            printf("It is not an UDP or TCP packet\n");
        }

    }
    else 
    {
        printf("It is not an IP packet\n");
        return;
    }
    return ;
}


int main(int argc, char **argv) 
{    
    if (argc != 3)
    {
        printf("please -r pcap_file\n");
        exit(1);
    }    
    char *name = argv[2];
    IP_packet = 0;    
    //char *device = NULL;
    char error_buffer[PCAP_ERRBUF_SIZE];
    pcap_t *handle;
    int snapshot_length = 1024;        /* Snapshot length is how many bytes to capture from each packet. This includes*/
    int total_packet_count = 0;       /* End the loop after this many packets are captured */
    u_char *my_arguments = NULL;
    //device = pcap_lookupdev(error_buffer);
    //handle = pcap_open_live(device, snapshot_length, 0, 10000, error_buffer);
    handle = pcap_open_offline(name,error_buffer);
    pcap_loop(handle, total_packet_count, my_packet_handler, my_arguments);
    return 0;
}
