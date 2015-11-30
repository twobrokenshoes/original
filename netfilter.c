#include <linux/init.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <uapi/linux/netfilter.h>
#include <linux/socket.h>/*PF_INET*/
#include <linux/netfilter_ipv4.h>/*NF_IP_PRE_FIRST*/
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/inet.h> /*in_aton()*/
#include <net/ip.h>
#include <net/tcp.h>
#include <net/udp.h>
#define ETHALEN 14
MODULE_LICENSE("GPL");
MODULE_AUTHOR("bbo");

struct nf_hook_ops nfho;

//#define	ETH_PACKAGE_DEBUG
#ifdef	ETH_PACKAGE_DEBUG
int eth_udp_debug(struct sk_buff *skb)
{
	unsigned char *pchar = NULL;
	int i = 0;
//	int data_len = 0;
	int count = 0;
	pchar = (unsigned char *)skb->network_header - ETHALEN;
#if 0
	printk("\nbefore eth");
	i = 0;
	while(pchar < ((unsigned char *)skb->network_header - ETHALEN))
	{
		if((i++%10) == 0)
		{
			printk("\n");
		}
		printk("%s%X ", (*pchar>0x0f)?"":"0", *pchar++);
		count++;
	}
#endif
	printk("\neth header ");
	i = 0;
	while(pchar < ((unsigned char *)skb->network_header))
	{
		if((i++%10) == 0)
		{
			printk("\n");
		}
		printk("%s%X ", (*pchar>0x0f)?"":"0", *pchar++);
		count++;
	}
	printk("\nip header ");
	i = 0;
	while(pchar < ((unsigned char *)skb->transport_header))
	{
		if((i++%10) == 0)
		{
			printk("\n");
		}
		printk("%s%X ", (*pchar>0x0f)?"":"0", *pchar++);
		count++;
	}
	printk("\nudp header ");
	i = 0;
	while(pchar < ((unsigned char *)skb->transport_header + 8))
	{
		if((i++%10) == 0)
		{
			printk("\n");
		}
		printk("%s%X ", (*pchar>0x0f)?"":"0", *pchar++);
		count++;
	}
	printk("\nudp data ");
	i = 0;
	while(pchar < ((unsigned char *)skb->tail))
	{
		if((i++%10) == 0)
		{
			printk("\n");
		}
		printk("%s%X ", (*pchar>0x0f)?"":"0", *pchar++);
		count++;
	}
#if 0
	printk("\nafter data");
	i = 0;
	while(pchar < ((unsigned char *)skb->end))
	{
		if((i++%10) == 0)
		{
			printk("\n");
		}
		printk("%s%X ", (*pchar>0x0f)?"":"0", *pchar++);
		count++;
	}
#endif
	printk("\n");
	return count;
}
#endif

#define IPPROTO	0x0008//htons(0x0800)
#define	MIN_DATA_LEN	(64 - 14 - 4)

struct machdr{
	unsigned char des_mac[6];
	unsigned char sou_mac[6];
	unsigned short protocal;
};

int send_udp_package(unsigned char *pdata, int len)
{
	int ret = 0;
	int	skb_len = 0;
	int data_len = 0;
    struct net_device *dev = NULL;
	struct udphdr *pudph = NULL;
	struct iphdr *piph = NULL;
	struct sk_buff *skb = NULL;
	unsigned char *pchar = NULL;
	static int id = 1;
	static struct machdr mach = {
		.des_mac[0] = 0xC8,
		.des_mac[1] = 0x60,
		.des_mac[2] = 0x00,
		.des_mac[3] = 0xB4,
		.des_mac[4] = 0x05,
		.des_mac[5] = 0xC3,
		
		.sou_mac[0] = 0x00,
		.sou_mac[1] = 0x09,
		.sou_mac[2] = 0xC0,
		.sou_mac[3] = 0xFF,
		.sou_mac[4] = 0xEC,
		.sou_mac[5] = 0x48,
		
		.protocal = IPPROTO,
	};
	static struct iphdr iph = {
		.ihl = 5,
		.version = 4,
//		.tot_len = 0x00,
		.tos = 0x00,
//		.id = id,
		.frag_off = 0x0040,//htons(0x4000),
		.ttl = 0x01,
		.protocol = 0x11,
		.check = 0x00,
		.saddr = 0x082a9b73,//in_aton("115.155.42.8"),
		.daddr = 0x092a9b73,//in_aton("115.155.42.9"),
	};
	static struct udphdr udph = {
		.source = 0xb822,//htons("8888"),
		.dest = 0xb922,//htons("8889"),
//		.len  = 8,
		.check= 0,
	};
	udph.check = 0x00;
	udph.len = htons(len+8);
	
	iph.check = 0x00;
	iph.id = htons(id);
	id++;
	iph.tot_len = htons(20 + 8 + len);
	
	skb_len = (ntohs(iph.tot_len) < MIN_DATA_LEN)?(66 + 14 + MIN_DATA_LEN + 40):(66 + 14 + ((ntohs(iph.tot_len) + 3)&(0xfffc)) + 40);
	skb = dev_alloc_skb((skb_len + 0x0f)&0xfff0);
	if(!skb)
	{
		printk("malloc skb failed \n");
		return -ENOMEM;
	}
    dev = dev_get_by_name(&init_net,"eth0");
	//setup udp data
	skb_reserve(skb, skb_len - 40);
	data_len = skb_len-66-14-20-8-40;
	skb_push(skb, data_len);
	pchar = (unsigned char *)skb->data;
	memset(pchar, 0, data_len);
	memcpy(pchar, pdata, len);
	
	//setup udp header
	skb_push(skb, 8);
	pchar = (unsigned char *)skb->data;
	pudph = (struct udphdr *)pchar;
	if(!pudph)
	{
		printk("failed to get udp header \n");
		ret = -1;
		goto header_err;
	}
	memcpy(pchar, &udph, 8);
	skb_reset_transport_header(skb);
	
	//setup ip header
	skb_push(skb, 20);
	pchar = (unsigned char *)skb->data;
	piph = (struct iphdr *)pchar;
	if(!piph)
	{
		printk("failed to get ip header \n");
		ret = -1;
		goto header_err;
	}
	memcpy(pchar, &iph, 20);
	skb_reset_network_header(skb);
	
	//setup mac header
	skb_push(skb, ETHALEN);
	pchar = (unsigned char *)skb->data;
	if(!pchar)
	{
		printk("failed to get mac header \n");
		ret = -1;
		goto header_err;
	}
	memcpy(pchar, &mach, ETHALEN);
	skb_reset_mac_header(skb);
	
	skb->csum = csum_partial((unsigned char *)pudph, ntohs(pudph->len),0);
	pudph->check = csum_tcpudp_magic(piph->saddr,
                        piph->daddr,
                        ntohs(piph->tot_len) - 20,piph->protocol,
                        skb->csum);
	piph->check = ip_fast_csum(piph,piph->ihl);

	skb->ip_summed = CHECKSUM_NONE;
	skb->pkt_type = PACKET_OTHERHOST;
	skb->dev = dev;
	ret = dev_queue_xmit(skb);
	if(ret < 0)
	{
		printk("failed to transfer the skb \n");
		ret = -1;
	}          
#ifdef	ETH_PACKAGE_DEBUG
	eth_udp_debug(skb);
#endif
header_err:
    dev = NULL;
	pudph = NULL;
	piph = NULL;
	pchar = NULL;
	dev_kfree_skb(skb);
	skb = NULL;
	return ret;
}
unsigned char message[] = {"abcdefghijklmnopqrstuvwxyz\n"};
unsigned char buff[] = {"I come from Hunan\n"};

unsigned int checksum(unsigned int hooknum,
                        struct sk_buff *__skb,
                        const struct net_device *in,
                        const struct net_device *out,
                        int (*okfn)(struct sk_buff *))
{
    struct sk_buff *skb;
    struct net_device *dev;
    struct iphdr *iph;
//    struct tcphdr *tcph;
	struct udphdr *udph;
    int tot_len;
    int iph_len;
    int tranport_header_len;
    int data_len = 0;
//    int ret;
    unsigned char *p = NULL;
//    int i;
    skb = __skb;
    if(skb == NULL)
        return NF_ACCEPT;
    iph = ip_hdr(skb);
    if(iph == NULL)
        return NF_ACCEPT;
    tot_len = ntohs(iph->tot_len);
    if(iph->daddr == in_aton("115.155.42.8"))
    {
        iph_len = ip_hdrlen(skb);/*in ip.h*/
        skb_pull(skb,iph_len);//skb->data指针定位到了传输层
        skb_reset_transport_header(skb);/*重置首部长度,现在的首部长度包括了的ip首部长度*/
        if(iph->protocol == IPPROTO_UDP)
        {
//            tcph = tcp_hdr(skb);
//            tranport_header_len = tcp_hdrlen(skb);
			udph = udp_hdr(skb);
			tranport_header_len = 8;
            if(udph->dest == htons(8888)) //根据自己得需求来进行过滤数据包
            {
				skb_pull(skb,tranport_header_len);
				data_len = tot_len - iph_len - tranport_header_len;
				if(data_len)
				{
					p = (unsigned char *)skb->data;
					while(data_len--)
					{
						printk("%c",*p++);
					}
					printk("\n");
				}
				skb_push(skb,tranport_header_len);
            
#if 0
				p = (unsigned char *)skb->mac_header;
				for(i = 0;i < 6; i++ )
				{
					p[i] = p[i]^p[6+i];
					p[6+i] = p[i]^p[6+i];
					p[i] = p[i]^p[6+i];
				}
				p = (unsigned char *)skb->transport_header + 8;
				*p = *p - 0x20;
                iph->saddr = in_aton("115.155.42.8");
                iph->daddr = in_aton("115.155.42.9");
                udph->dest= htons(8889);
//                udph->source= htons(8887);
                dev = dev_get_by_name(&init_net,"eth0");
                udph->check = 0;
                skb->csum = csum_partial((unsigned char *)udph, tot_len - iph_len,0);
                udph->check = csum_tcpudp_magic(iph->saddr,
                        iph->daddr,
                        ntohs(iph->tot_len) - iph_len,iph->protocol,
                        skb->csum);
                iph->check = 0;
                iph->check = ip_fast_csum(iph,iph->ihl);
    
                skb->ip_summed = CHECKSUM_NONE;
                skb->pkt_type = PACKET_OTHERHOST;
                skb->dev = dev;
                skb_push(skb,iph_len);/*在返回之前，先将skb中得信息恢复至原始L3层状态*/
                //skb_reset_transport_header(skb);
                skb_push(skb, ETHALEN);//将skb->data指向l2层，之后将数据包通过dev_queue_xmit()发出
                
#ifdef	ETH_PACKAGE_DEBUG
				eth_udp_debug(skb);
#endif
                ret = dev_queue_xmit(skb);
                if(ret < 0)
                {
                    printk("dev_queue_xmit() error\n");
                    goto out;
                }
                return NF_STOLEN;
#endif
				send_udp_package(message,strlen(message));
            }
     }
        skb_push(skb,iph_len);/*在返回之前，先将skb中得信息恢复至原始L3层状态*/
        skb_reset_transport_header(skb);
    }
    
    return NF_ACCEPT;
out:
    dev_put(dev);
    //free(skb);
    return NF_DROP;
}
static int __init filter_init(void)
{
    int ret;
        nfho.hook = checksum;
        nfho.pf = AF_INET;
        nfho.hooknum = NF_INET_PRE_ROUTING;
        nfho.priority = NF_IP_PRI_FIRST;
    
        ret = nf_register_hook(&nfho);
        if(ret < 0)
        {
            printk("%s\n", "can't modify skb hook!");
            return ret;
        }
    return 0;
}
static void filter_fini(void)
{
    nf_unregister_hook(&nfho);
}
module_init(filter_init);
module_exit(filter_fini);
