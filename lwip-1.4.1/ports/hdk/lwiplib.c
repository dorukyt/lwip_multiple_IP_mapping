/**
*  \file lwiplib.c
*
*  \brief lwip related initializations
*/
/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
*/

/*
** Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
** ALL RIGHTS RESERVED
*/

/*
** lwIP Compile Time Options for HDK.
*/
#include "lwipopts.h"
#include "lwiplib.h"
#include <string.h>   /* memcpy() -- alias netif icin eklendi */

/*
** lwIP high-level API/Stack/IPV4/SNMP/Network Interface/PPP codes
*/
#include "src/api/api_lib.c"
#include "src/api/api_msg.c"
#include "src/api/err.c"
#include "src/api/netbuf.c"
#include "src/api/netdb.c"
#include "src/api/netifapi.c"
#include "src/api/tcpip.c"
#include "src/api/sockets.c"

#include "src/core/dhcp.c"
#include "src/core/dns.c"
#include "src/core/init.c"
#include "src/core/mem.c"
#include "src/core/memp.c"
#include "src/core/netif.c"
#include "src/core/pbuf.c"
#include "src/core/raw.c"
#include "src/core/stats.c"
#include "src/core/sys.c"
#include "src/core/tcp.c"
#include "src/core/tcp_in.c"
#include "src/core/tcp_out.c"
#include "src/core/udp.c"

#include "src/core/ipv4/autoip.c"
#include "src/core/ipv4/icmp.c"
#include "src/core/ipv4/igmp.c"
#include "src/core/ipv4/inet.c"
#include "src/core/ipv4/inet_chksum.c"
#include "src/core/ipv4/ip.c"
#include "src/core/ipv4/ip_addr.c"
#include "src/core/ipv4/ip_frag.c"

#include "src/core/snmp/asn1_dec.c"
#include "src/core/snmp/asn1_enc.c"
#include "src/core/snmp/mib2.c"
#include "src/core/snmp/mib_structs.c"
#include "src/core/snmp/msg_in.c"
#include "src/core/snmp/msg_out.c"

#include "src/netif/etharp.c"
#include "src/netif/loopif.c"

#include "src/netif/ppp/auth.c"
#include "src/netif/ppp/chap.c"
#include "src/netif/ppp/chpms.c"
#include "src/netif/ppp/fsm.c"
#include "src/netif/ppp/ipcp.c"
#include "src/netif/ppp/lcp.c"
#include "src/netif/ppp/magic.c"
#include "src/netif/ppp/md5.c"
#include "src/netif/ppp/pap.c"
#include "src/netif/ppp/ppp.c"
#include "src/netif/ppp/ppp_oe.c"
#include "src/netif/ppp/randm.c"
#include "src/netif/ppp/vj.c"

/*
** HDK-specific lwIP interface/porting layer code.
*/
#include "ports/hdk/perf.c"
#include "ports/hdk/sys_arch.c"
#include "ports/hdk/netif/hdkif.c"
#include "locator.c"

/******************************************************************************
**                       INTERNAL VARIABLE DEFINITIONS
******************************************************************************/
/*
** The lwIP network interface structure for the HDK Ethernet MAC.
*/
static struct netif hdkNetIF[MAX_EMAC_INSTANCE];

/******************************************************************************
**                          FUNCTION DEFINITIONS
******************************************************************************/
/**
 *
 * \brief Initializes the lwIP TCP/IP stack.
 *
 * \param instNum  The instance index of the EMAC module
 * \param macArray Pointer to the MAC Address
 * \param ipAddr   The IP address to be used 
 * \param netMask  The network mask to be used 
 * \param gwAddr   The Gateway address to be used 
 * \param ipMode   The IP Address Mode.
 *        ipMode can take the following values\n
 *             IPADDR_USE_STATIC - force static IP addressing to be used \n
 *             IPADDR_USE_DHCP - force DHCP with fallback to Link Local \n
 *             IPADDR_USE_AUTOIP - force  Link Local only
 *
 * This function performs initialization of the lwIP TCP/IP stack for the
 * HDK EMAC, including DHCP and/or AutoIP, as configured.
 *
 * NOT (bu projede eklendi): Bu fonksiyon HALA MEVCUT ve eski davranisini
 * koruyor (geriye donuk uyumluluk icin). Ancak coklu-IP / IP aliasing
 * senaryosu icin bunun yerine asagidaki YENI fonksiyonlari kullanin:
 *   lwIPCoreInit()   -> donanim + lwip_init(), SADECE BIR KERE
 *   lwIPNetifAdd()   -> birincil (fiziksel) IP
 *   lwIPAliasAdd()   -> ikinci/sanal IP (donanima dokunmaz)
 * lwIPInit()'i lwIPCoreInit()/lwIPNetifAdd() ile KARISTIRMAYIN: ayni
 * instNum uzerinde ikisini birden cagirmak Iteration-1'deki (memory
 * corruption / link drop) sorunu geri getirir.
 *
 * \return IP Address.
*/
 unsigned int lwIPInit(unsigned int instNum, unsigned char *macArray,
                       unsigned int ipAddr, unsigned int netMask,
                       unsigned int gwAddr, unsigned int ipMode)
 {
     struct ip_addr ip_addr;
     struct ip_addr net_mask;
     struct ip_addr gw_addr;
     volatile unsigned char *state;
     unsigned int *ipAddrPtr;
     volatile unsigned int cnt = 0x3FFFFFFF;

     lwip_init();

     /* Setup the network address values. */
     if(ipMode == IPADDR_USE_STATIC)
     {
         ip_addr.addr = htonl(ipAddr);
         net_mask.addr = htonl(netMask);
         gw_addr.addr = htonl(gwAddr);
     }

     else
     {
         ip_addr.addr = 0;
         net_mask.addr = 0;
         gw_addr.addr = 0;
     }

     hdkif_macaddrset(instNum, macArray);

     /*
     ** Create, configure and add the Ethernet controller interface with
     ** default settings.  ip_input should be used to send packets directly to
     ** the stack. The lwIP will internaly call the hdkif_init function.
     */
     if(NULL ==
        netif_add(&hdkNetIF[instNum], &ip_addr, &net_mask, &gw_addr, &instNum,
               hdkif_init, ip_input))
     {
         return 0;
     };

     netif_set_default(&hdkNetIF[instNum]);

     /* Start DHCP, if enabled. */
 #if LWIP_DHCP
 if(ipMode == IPADDR_USE_DHCP)
 {
 	unsigned int dhcp_flag = 0;
 	unsigned int dhcp_tries = 5;
 	unsigned int count;
 	unsigned int delay;
 	while(dhcp_tries--)
 	{


 	dhcp_start(&hdkNetIF[instNum]);
 	count = 10;
 	/* Check for DHCP completion for 'count' number of times, each for the given delay. */
 		while(count--)
 		{
 			delay = 0x8FFFFU;
 			while(delay--);
 			state = &(hdkNetIF[instNum].dhcp->state);
 			if(DHCP_BOUND == *state)
 			{
 				dhcp_flag = 1;
 			    ipAddrPtr = (unsigned int*)&(hdkNetIF[instNum].ip_addr);
 			    return (*ipAddrPtr);
 			}
 		}

 	}
 	/* In case of DHCP failure, return 0. */
 	if(dhcp_flag == 0)
 		return 0;
 }
 #endif

     /* Start AutoIP, if enabled and DHCP is not. */
 #if LWIP_AUTOIP
     if(ipMode == IPADDR_USE_AUTOIP)
     {
         autoip_start(&hdkNetIF[instNum]);
     }
 #endif

     if((ipMode == IPADDR_USE_STATIC)
     	       ||(ipMode == IPADDR_USE_AUTOIP))
     {
        /* Bring the interface up */
        netif_set_up(&hdkNetIF[instNum]);
     }

     ipAddrPtr = (unsigned int*)&(hdkNetIF[instNum].ip_addr);

     return (*ipAddrPtr);
 }

/*
 * \brief   Checks if the ethernet link is up
 *
 * \param   instNum  The instance number of EMAC module 
 *
 * \return  Interface status.
*/
unsigned int lwIPNetIfStatusGet(unsigned int instNum) 
{
    
    return (hdkif_netif_status(&hdkNetIF[instNum]));
}

/*
 * \brief   Checks if the ethernet link is up
 *
 * \param   instNum  The instance number of EMAC module 
 *
 * \return  The link status.
*/
unsigned int lwIPLinkStatusGet(unsigned int instNum) 
{
    return (hdkif_link_status(&hdkNetIF[instNum]));     
}

/**
 * \brief   Interrupt handler for Receive Interrupt. Directly calls the
 *          HDK interface receive interrupt handler.
 *
 * \param   instNum  The instance number of EMAC module for which receive 
 *                   interrupt happened
 *
 * \return  None.
*/
void lwIPRxIntHandler(unsigned int instNum) 
{
    hdkif_rx_inthandler(&hdkNetIF[instNum]);
}

/**
 * \brief   Interrupt handler for Transmit Interrupt. Directly calls the 
 *          HDK interface transmit interrupt handler.
 *
 * \param   instNum  The instance number of EMAC module for which transmit
 *                   interrupt happened
 *
 * \return  None.
*/
void lwIPTxIntHandler(unsigned int instNum)
{
    hdkif_tx_inthandler(&hdkNetIF[instNum]);
}

/******************************************************************************
**             MODULAR MULTI-IP / IP-ALIASING EXTENSION (EKLENDI)
**
** Asagidaki 3 fonksiyon, TI'nin lwIPInit()'ini BOLEREK ayni hedefe
** modular sekilde ulasir:
**
**   lwIPInit()        =  lwIPCoreInit()  +  lwIPNetifAdd()
**                         (+ DHCP/AutoIP, bunlar aliasta anlamsiz oldugu
**                          icin dahil edilmedi; static IP odakli)
**
** lwIPCoreInit() ve lwIPNetifAdd() ayri fonksiyonlar oldugu icin,
** hdkif_init() (donanim reset/PHY autoneg) SADECE lwIPNetifAdd()
** cagrildiginda, yani birincil arayuz icin BIR KERE tetiklenir.
** lwIPAliasAdd() ise donanima hic dokunmayan bir init callback
** (hdkif_alias_init) kullanarak ikinci IP'yi ayni hdkNetIF[instNum]
** cikislarini paylasacak sekilde netif_list'e ekler.
******************************************************************************/

#define MAX_ALIAS_NETIF     3U

static struct netif  hdkAliasNetIF[MAX_ALIAS_NETIF];
static unsigned int  aliasNetifCount   = 0U;
static unsigned int  lwIPCoreReady     = 0U;
/* netif_add() init callback'i SENKRON olarak (netif_add donmeden once)
 * cagirdigi icin bu gecici pointer race condition olusturmaz (NO_SYS /
 * bare-metal, tek thread). */
static struct netif  *aliasSourceNetif = NULL;

/**
 * \brief   Donanimi (MAC adres register yazimi dahil) ve lwIP core'unu
 *          SADECE BIR KERE baslatir. Ikinci cagrida no-op doner --
 *          Iteration-1'deki "lwIPInit() iki kere cagrilinca memp/pbuf
 *          havuzlari ve PHY link-state bozuluyor" sorununu yapisal
 *          olarak imkansiz hale getirir.
 *
 * \param   instNum   EMAC modul instance indeksi (TMS570LC4357'de 0).
 * \param   macArray  6 byte fiziksel MAC adresi.
 *
 * \return  1 basarili, 0 zaten baslatilmisti (hala basarili sayilir).
 */
unsigned int lwIPCoreInit(unsigned int instNum, unsigned char *macArray)
{
    if (lwIPCoreReady)
    {
        return 1U;
    }

    lwip_init();

    /* Fiziksel MAC adresini silikona yaz -- Iteration-3'teki "invisible
     * ping" sorununun cozumu: bu satir olmadan EMAC hardware address
     * filtering ARP paketlerini sessizce dusurur. */
    hdkif_macaddrset(instNum, macArray);

    lwIPCoreReady = 1U;
    return 1U;
}

/**
 * \brief   Birincil (fiziksel donanimi sahiplenen) netif'i ekler.
 *          hdkif_init() burada, SADECE burada calisir -- PHY reset/
 *          autonegotiation bu cagriya mahsustur.
 *
 * \param   instNum      EMAC instance indeksi (lwIPCoreInit ile ayni).
 * \param   ipAddr, netMask, gwAddr   Host byte order (fonksiyon icinde
 *                                    htonl uygulanir; BE-8 derleyicide
 *                                    bu no-op'tur, sorun degil).
 * \param   makeDefault  1 ise bu netif default route olur.
 *
 * \return  IP adresi (basarili), 0 (hata: once lwIPCoreInit() cagirin
 *          veya netif_add basarisiz oldu).
 */
unsigned int lwIPNetifAdd(unsigned int instNum, unsigned int ipAddr,
                          unsigned int netMask, unsigned int gwAddr,
                          unsigned int makeDefault)
{
    struct ip_addr ip_addr;
    struct ip_addr net_mask;
    struct ip_addr gw_addr;
    unsigned int  *ipAddrPtr;

    if (!lwIPCoreReady)
    {
        return 0U;   /* once lwIPCoreInit() cagrilmali */
    }

    ip_addr.addr  = htonl(ipAddr);
    net_mask.addr = htonl(netMask);
    gw_addr.addr  = htonl(gwAddr);

    if (NULL == netif_add(&hdkNetIF[instNum], &ip_addr, &net_mask, &gw_addr,
                           &instNum, hdkif_init, ip_input))
    {
        return 0U;
    }

    if (makeDefault)
    {
        netif_set_default(&hdkNetIF[instNum]);
    }

    netif_set_up(&hdkNetIF[instNum]);

    ipAddrPtr = (unsigned int *)&(hdkNetIF[instNum].ip_addr);
    return (*ipAddrPtr);
}

/**
 * \brief   Dummy/alias init callback. hdkif_init()'in aksine PHY/MDIO/
 *          EMAC register'larina HICBIR SEKILDE dokunmaz. Sadece
 *          birincil netif'ten .output / .linkoutput fonksiyon
 *          pointer'larini ve MAC adres bilgisini kopyalar. Boylece tek
 *          fiziksel descriptor ring + CPDMA interrupt'lari, TX
 *          tarafinda hem ana hem sanal IP icin ayni cikisi kullanir.
 */
static err_t hdkif_alias_init(struct netif *netif)
{
    if (aliasSourceNetif == NULL)
    {
        return ERR_ARG;
    }

    /* KRITIK FIX: netif->state, hdkif_output()/hdkif_transmit() tarafindan
     * "struct hdkif *" olarak kullanilir (TX descriptor ring, emac_base vs.
     * buradadir). Bu satir olmadan alias netif->state NULL kalir;
     * hdkif_transmit() NULL+offset adresine "curr_bd->flags_pktlen = ..."
     * yazmaya calisir -> board kilitlenir. Alias fiziksel donanimi
     * paylastigi icin AYNI hdkif struct'ina isaret etmeli. */
    netif->state = aliasSourceNetif->state;

    netif->output     = aliasSourceNetif->output;
    netif->linkoutput = aliasSourceNetif->linkoutput;
    netif->mtu        = aliasSourceNetif->mtu;
    netif->hwaddr_len = aliasSourceNetif->hwaddr_len;
    memcpy(netif->hwaddr, aliasSourceNetif->hwaddr, netif->hwaddr_len);
    netif->flags      = aliasSourceNetif->flags;

    return ERR_OK;
}

/**
 * \brief   Ayni fiziksel EMAC/MAC uzerinde IKINCI bir statik IP
 *          (alias) ekler. Donanima dokunmaz; sadece netif_list'e yeni
 *          bir dugum ekler.
 *
 * \param   primaryInstNum  lwIPNetifAdd() ile eklenmis birincil
 *                          arayuzun instNum degeri.
 * \param   ipAddr, netMask, gwAddr  Sanal IP bilgileri (host byte
 *                                   order; icerde htonl uygulanir).
 *
 * \return  Sanal IP adresi (basarili), 0 (hata).
 *
 * \note    ONEMLI ACIK KONU (asagidaki "ARP notu"na bakin): Bu
 *          fonksiyon IP-katmani teslimatini (ip_input netif_list
 *          taramasi) ve TX cikisini dogru sekilde paylastirir. ANCAK
 *          etharp.c icindeki etharp_arp_input()'un ARP-REPLY
 *          uretirken netif_list'i mi yoksa sadece kendisine verilen
 *          tek netif'i mi kontrol ettigi, lwIP 1.4.1'in etharp.c
 *          kaynagina bagli. Sizin etharp.c dosyaniz henuz elimde
 *          olmadigi icin bu satiri %100 dogrulayamiyorum -- asagidaki
 *          yaniti okuyun.
 */
unsigned int lwIPAliasAdd(unsigned int primaryInstNum, unsigned int ipAddr,
                          unsigned int netMask, unsigned int gwAddr)
{
    struct ip_addr ip_addr;
    struct ip_addr net_mask;
    struct ip_addr gw_addr;
    struct netif  *aliasNetif;
    unsigned int  *ipAddrPtr;

    if ((!lwIPCoreReady) || (aliasNetifCount >= MAX_ALIAS_NETIF))
    {
        return 0U;
    }

    ip_addr.addr  = htonl(ipAddr);
    net_mask.addr = htonl(netMask);
    gw_addr.addr  = htonl(gwAddr);

    aliasNetif = &hdkAliasNetIF[aliasNetifCount];

    aliasSourceNetif = &hdkNetIF[primaryInstNum];

    if (NULL == netif_add(aliasNetif, &ip_addr, &net_mask, &gw_addr,
                           NULL, hdkif_alias_init, ip_input))
    {
        aliasSourceNetif = NULL;
        return 0U;
    }

    aliasSourceNetif = NULL;

    netif_set_up(aliasNetif);
    aliasNetifCount++;

    ipAddrPtr = (unsigned int *)&(aliasNetif->ip_addr);
    return (*ipAddrPtr);
}

/***************************** End Of File ***********************************/
