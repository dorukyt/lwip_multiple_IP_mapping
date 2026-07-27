/*
 * ping.c
 *
 * ICMP echo request (ping) over the lwIP RAW api.
 *
 * TX: paket elle kurulup dogrudan ip_output_if()'e verilir; boylece
 *     kaynak IP + gateway secimi tamamen cagiranin verdigi netif'ten
 *     olur (lwIP 1.4.1'de raw_sendto_if yok).
 * RX: tek bir raw pcb, icmp_input'tan ONCE tum gelen ICMP paketlerini
 *     gorur; sadece bizim id/seqno'muza uyan echo reply tuketilir,
 *     digerleri stack'e geri birakilir (kartin kendi ping cevaplayicisi
 *     calismaya devam eder).
 * Zaman: RTI FRC0 (9.375 MHz) ile RTT olcumu ve timeout beklemesi.
 */

#include "lwip/opt.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/ip.h"
#include "lwip/inet_chksum.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"

#include "HL_rti.h"

#include "ping.h"

/* RTI FRC0: 9.375 MHz -> 9375 tick = 1 ms
 * (HL_notification.c debounce: 1406250 tick ~= 150 ms ile tutarli) */
#define PING_RTI_TICKS_PER_MS  9375U

/* echo request id: reply'da aynen geri doner, bizim paketimizi ayirt eder */
#define PING_ID  0xAFAF

static struct raw_pcb *s_ping_pcb = NULL;
static u16_t s_seq = 0;

/* tek bekleyen istek durumu; EMAC RX ISR'i tarafindan yazilir */
static volatile u8_t s_waiting = 0;
static volatile u8_t s_reply_valid = 0;
static volatile u32_t s_send_tick = 0;
static volatile ping_result_t s_result;

/* ISR baglaminda calisir: raw pcb'ye gelen her ICMP paketi buradan gecer.
 * pbuf IP header ile birlikte gelir. return 1 = paket bizim, tuketildi;
 * return 0 = stack islemeye devam etsin. */
static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p,
                      ip_addr_t *addr)
{
    struct ip_hdr *iphdr;
    struct icmp_echo_hdr *iecho;
    s16_t ip_hlen;

    if (0 == s_waiting || 1 == s_reply_valid)
    {
        return 0;
    }

    iphdr = (struct ip_hdr *)p->payload;
    ip_hlen = (s16_t)(IPH_HL(iphdr) * 4);

    if (p->tot_len < (u16_t)(ip_hlen + sizeof(struct icmp_echo_hdr)))
    {
        return 0;
    }

    /* IP header'i gizle, ICMP header'a bak */
    if (pbuf_header(p, (s16_t)-ip_hlen) == 0)
    {
        iecho = (struct icmp_echo_hdr *)p->payload;

        if ((ICMPH_TYPE(iecho) == ICMP_ER)
                && (iecho->id == PING_ID)
                && (iecho->seqno == htons(s_seq)))
        {
            u32_t now = rtiREG1->CNT[0U].FRCx;
            ping_result_t *res = (ping_result_t *)&s_result;

            ip_addr_set(&res->from, addr);
            res->seqno = s_seq;
            res->data_len = (u16_t)(p->tot_len - sizeof(struct icmp_echo_hdr));
            res->rtt_ms = (now - s_send_tick) / PING_RTI_TICKS_PER_MS;
            res->ttl = IPH_TTL(iphdr);

            s_waiting = 0;
            s_reply_valid = 1;

            pbuf_free(p);
            return 1;   /* paketi biz yedik */
        }

        /* bizim degil: IP header'i geri koy, stack'e birak */
        pbuf_header(p, ip_hlen);
    }
    return 0;
}

err_t ping_send(struct netif *netif, ip_addr_t *target)
{
    struct pbuf *p;
    struct icmp_echo_hdr *iecho;
    u16_t len = (u16_t)(sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE);
    u16_t i;
    err_t err;
    SYS_ARCH_DECL_PROTECT(lev);

    if (NULL == netif || NULL == target)
    {
        return ERR_VAL;
    }

    /* ilk kullanimda RX icin raw pcb'yi kur (IP_PROTO_ICMP = 1) */
    if (NULL == s_ping_pcb)
    {
        s_ping_pcb = raw_new(IP_PROTO_ICMP);
        if (NULL == s_ping_pcb)
        {
            return ERR_MEM;
        }
        raw_recv(s_ping_pcb, ping_recv, NULL);
    }

    p = pbuf_alloc(PBUF_IP, len, PBUF_RAM);
    if (NULL == p)
    {
        return ERR_MEM;
    }

    /* ICMP echo request'i elle kur */
    iecho = (struct icmp_echo_hdr *)p->payload;
    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id = PING_ID;
    s_seq++;
    iecho->seqno = htons(s_seq);

    for (i = 0; i < PING_DATA_SIZE; i++)
    {
        ((u8_t *)p->payload)[sizeof(struct icmp_echo_hdr) + i] =
                (u8_t)('a' + (i % 23));
    }

    /* raw gonderimde checksum'i uygulama hesaplar */
    iecho->chksum = inet_chksum(iecho, len);

    SYS_ARCH_PROTECT(lev);
    s_reply_valid = 0;
    s_send_tick = rtiREG1->CNT[0U].FRCx;
    s_waiting = 1;

    /* kaynak IP + cikis netif'i zorla: udp_sendto_if'in ICMP karsiligi */
    err = ip_output_if(p, &netif->ip_addr, target, ICMP_TTL, 0,
                       IP_PROTO_ICMP, netif);
    SYS_ARCH_UNPROTECT(lev);

    if (err != ERR_OK)
    {
        s_waiting = 0;
    }

    pbuf_free(p);
    return err;
}

u8_t ping_wait_reply(u32_t timeout_ms, ping_result_t *out)
{
    u32_t start = rtiREG1->CNT[0U].FRCx;
    u32_t timeout_ticks = timeout_ms * PING_RTI_TICKS_PER_MS;
    SYS_ARCH_DECL_PROTECT(lev);

    while ((rtiREG1->CNT[0U].FRCx - start) < timeout_ticks)
    {
        if (1 == s_reply_valid)
        {
            SYS_ARCH_PROTECT(lev);
            if (out != NULL)
            {
                *out = *(ping_result_t *)&s_result;
            }
            s_reply_valid = 0;
            SYS_ARCH_UNPROTECT(lev);
            return 1;
        }
    }

    /* timeout: gec gelen reply artik eslesmesin */
    s_waiting = 0;
    return 0;
}
