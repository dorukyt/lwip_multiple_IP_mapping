/*
 * network_diagnostics.c
 *
 * Realistic ICMP ping diagnostics for TMS570LC4357 + lwIP 1.4.1
 *
 * Gercek bir ping isleminde iki asama vardir:
 *
 *   1. ARP  — PC "192.168.2.44 kimin?" diye sorar, board ARP Reply gonderir.
 *   2. ICMP — PC ping paketi gonderir, board Echo Reply ile cevaplar.
 *
 * Bu modul HER IKI asamayi da simule eder ve sonuclari ayri ayri raporlar.
 * Hook noktasi: netif->linkoutput (donanim TX hemen oncesi — en alt
 * yazilim katmani). Hem ARP Reply hem ICMP Reply bu noktadan gecer.
 *
 *  Created on: 10 Tem 2026
 *      Author: doruk_yaman
 */

/* ===================================================================== */
/*  INCLUDES                                                              */
/* ===================================================================== */

#include "network_diagnostics.h"

#include <string.h>        /* memcpy, memset, strlen */
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/ip_addr.h"
#include "netif/etharp.h"  /* ethernet_input() */
#include "HL_sci.h"

extern void sciDisplayText(sciBASE_t *sci, uint8_t *text, uint32_t length);

/* String literal'i UART'a bas.  SADECE "..." ile kullanin, degiskenle degil. */
#define UART_STR(s)  sciDisplayText(sciREG1, (uint8_t*)(s), \
                                    (uint32_t)(sizeof(s) - 1))

/* ===================================================================== */
/*  CHECKSUM (RFC 1071) — ENDIAN-SAFE                                     */
/* ===================================================================== */

static uint16_t inet_cksum(const uint8_t *data, uint16_t len)
{
    uint32_t sum = 0;
    uint16_t i;

    for (i = 0; i + 1 < len; i += 2)
        sum += ((uint32_t)data[i] << 8) | data[i + 1];

    if (len & 1)
        sum += (uint32_t)data[len - 1] << 8;

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)(~sum);
}

/* ===================================================================== */
/*  IP ADDRESS HELPER                                                     */
/* ===================================================================== */

/**
 * lwIP ip_addr_t'nin 4 oktetini network byte order'da cikarir.
 * Hem big-endian hem little-endian CPU'da dogru calisir.
 */
static void get_ip_bytes(const ip_addr_t *addr, uint8_t *out)
{
    const uint8_t *raw = (const uint8_t *)&(addr->addr);
    out[0] = raw[0];
    out[1] = raw[1];
    out[2] = raw[2];
    out[3] = raw[3];
}

/* ===================================================================== */
/*  LINKOUTPUT CAPTURE HOOK                                               */
/* ===================================================================== */
/*
 * netif->linkoutput, yazilimin donanima frame gonderdigi SON noktadir.
 * Buraya gelen her frame tam Ethernet cercevesidir (header dahil).
 * Hem ARP Reply hem ICMP Reply bu noktadan gecer, dolayisiyla
 * tek bir hook ile ikisini de yakalayabiliriz.
 *
 * Frame yapisi (linkoutput seviyesinde):
 *
 *   ARP Reply:
 *     byte[12-13] = EtherType 0x0806
 *     byte[20-21] = ARP Opcode 0x0002 (Reply)
 *
 *   ICMP Echo Reply:
 *     byte[12-13] = EtherType 0x0800
 *     byte[23]    = IP Protocol 0x01 (ICMP)
 *     byte[34]    = ICMP Type 0x00 (Echo Reply)
 */

#define DIAG_MAX_NETIFS  4

static struct {
    struct netif          *nif;
    netif_linkoutput_fn    orig;
} s_link_saved[DIAG_MAX_NETIFS];
static int s_link_cnt = 0;

/** ARP Reply yakalandi mi? */
static volatile int s_arp_reply_seen  = 0;

/** ICMP Echo Reply yakalandi mi? */
static volatile int s_icmp_reply_seen = 0;

static err_t capture_linkoutput(struct netif *netif, struct pbuf *p)
{
    if (p != NULL && p->tot_len >= 42) {
        uint8_t *d = (uint8_t *)p->payload;
        uint16_t etype = ((uint16_t)d[12] << 8) | d[13];

        if (etype == 0x0806) {
            /* ARP frame — opcode at byte 20-21 */
            uint16_t opcode = ((uint16_t)d[20] << 8) | d[21];
            if (opcode == 0x0002)           /* ARP Reply */
                s_arp_reply_seen = 1;
        }
        else if (etype == 0x0800) {
            /* IPv4 frame — protocol at byte 23, ICMP type at byte 34 */
            if (d[23] == 0x01 && d[34] == 0x00)  /* ICMP Echo Reply */
                s_icmp_reply_seen = 1;
        }
    }

    /* Orijinal linkoutput'u cagir (donanim TX).
     * Kablosuz boardda EMAC frame'i kabul eder ama hatta gondermez. */
    {
        int i;
        for (i = 0; i < s_link_cnt; i++) {
            if (s_link_saved[i].nif == netif && s_link_saved[i].orig != NULL)
                return s_link_saved[i].orig(netif, p);
        }
    }
    return ERR_OK;
}

static void hook_linkoutputs(void)
{
    struct netif *n;
    s_link_cnt = 0;
    for (n = netif_list; n != NULL && s_link_cnt < DIAG_MAX_NETIFS;
         n = n->next)
    {
        s_link_saved[s_link_cnt].nif  = n;
        s_link_saved[s_link_cnt].orig = n->linkoutput;
        n->linkoutput = capture_linkoutput;
        s_link_cnt++;
    }
}

static void restore_linkoutputs(void)
{
    int i;
    for (i = 0; i < s_link_cnt; i++)
        s_link_saved[i].nif->linkoutput = s_link_saved[i].orig;
    s_link_cnt = 0;
}

/* ===================================================================== */
/*  PACKET BUILDERS                                                       */
/* ===================================================================== */

/* ----- ARP Request (42 bytes) ---------------------------------------- */
/*
 *  [Ethernet 14][ARP 28]
 *
 *  Ethernet: dst=FF:FF:FF:FF:FF:FF (broadcast), src=sender, type=0x0806
 *  ARP:      HW=Ethernet, Proto=IPv4, Op=Request
 *            Sender MAC/IP = sahte gonderici
 *            Target MAC=00:00:00:00:00:00, Target IP = sorgulanacak IP
 */
#define ARP_FRAME_LEN  42

static void build_arp_request(uint8_t *buf, const ping_test_params_t *prm)
{
    /* Ethernet header */
    memset(&buf[ 0], 0xFF, 6);                   /* dst = broadcast      */
    memcpy(&buf[ 6], prm->src_mac, 6);           /* src = sender         */
    buf[12] = 0x08;  buf[13] = 0x06;             /* EtherType = ARP      */

    /* ARP header */
    buf[14] = 0x00;  buf[15] = 0x01;             /* HW type = Ethernet   */
    buf[16] = 0x08;  buf[17] = 0x00;             /* Proto type = IPv4    */
    buf[18] = 0x06;                               /* HW size = 6         */
    buf[19] = 0x04;                               /* Proto size = 4      */
    buf[20] = 0x00;  buf[21] = 0x01;             /* Opcode = Request     */
    memcpy(&buf[22], prm->src_mac, 6);           /* Sender MAC           */
    memcpy(&buf[28], prm->src_ip,  4);           /* Sender IP            */
    memset(&buf[32], 0x00, 6);                    /* Target MAC = unknown */
    memcpy(&buf[38], prm->dst_ip,  4);           /* Target IP            */
}

/* ----- ICMP Echo Request (74 bytes) ---------------------------------- */
/*
 *  [Ethernet 14][IPv4 20][ICMP 8][Payload 32]
 */
#define PING_FRAME_LEN  74

static void build_ping_frame(uint8_t *buf, const ping_test_params_t *prm)
{
    uint16_t ck;
    int i;

    /* Ethernet header (14 B) */
    memcpy(&buf[ 0], prm->dst_mac, 6);
    memcpy(&buf[ 6], prm->src_mac, 6);
    buf[12] = 0x08;  buf[13] = 0x00;             /* EtherType = IPv4     */

    /* IPv4 header (20 B, offset 14) */
    buf[14] = 0x45;                               /* Ver=4, IHL=5        */
    buf[15] = 0x00;
    buf[16] = 0x00;  buf[17] = 0x3C;             /* Total Length = 60   */
    buf[18] = 0x12;  buf[19] = 0x34;             /* Identification      */
    buf[20] = 0x00;  buf[21] = 0x00;             /* Flags / Frag Off    */
    buf[22] = 0x40;                               /* TTL = 64            */
    buf[23] = 0x01;                               /* Protocol = ICMP     */
    buf[24] = 0x00;  buf[25] = 0x00;             /* Checksum (placeholder) */
    memcpy(&buf[26], prm->src_ip, 4);
    memcpy(&buf[30], prm->dst_ip, 4);

    ck = inet_cksum(&buf[14], 20);
    buf[24] = (uint8_t)(ck >> 8);
    buf[25] = (uint8_t)(ck & 0xFF);

    /* ICMP Echo Request (8 B, offset 34) */
    buf[34] = 0x08;                               /* Type = Echo Request */
    buf[35] = 0x00;                               /* Code = 0            */
    buf[36] = 0x00;  buf[37] = 0x00;             /* Checksum (placeholder) */
    buf[38] = 0x00;  buf[39] = 0x01;             /* Identifier          */
    buf[40] = 0x00;  buf[41] = 0x01;             /* Sequence            */

    /* Payload (32 B, offset 42) */
    for (i = 0; i < 32; i++)
        buf[42 + i] = (uint8_t)(0x61 + i);

    ck = inet_cksum(&buf[34], 40);
    buf[36] = (uint8_t)(ck >> 8);
    buf[37] = (uint8_t)(ck & 0xFF);
}

/* ===================================================================== */
/*  PUBLIC: REALISTIC PING TEST                                           */
/* ===================================================================== */

void inject_realistic_ping_test(const ping_test_params_t *params,
                                struct netif *input_netif,
                                int *arp_result,
                                int *icmp_result)
{
    struct pbuf *pb;

    *arp_result  = 0;
    *icmp_result = 0;

    /* ---- Hook linkoutput on ALL netifs ---- */
    s_arp_reply_seen  = 0;
    s_icmp_reply_seen = 0;
    hook_linkoutputs();

    /* ============================================================
     *  ADIM 1 — ARP Request:  "dst_ip kimin? MAC'ini soyle."
     *
     *  Gercek hayat: PC bunu gonderir.
     *  etharp_arp_input() hedef IP'yi SADECE paketin geldigi
     *  netif'in IP'si ile karsilastirir.  Eger eslesirse
     *  ARP Reply gonderir; eslesmezse sessizce duser.
     * ============================================================ */
    {
        uint8_t arp_frame[ARP_FRAME_LEN];
        build_arp_request(arp_frame, params);

        pb = pbuf_alloc(PBUF_RAW, ARP_FRAME_LEN, PBUF_POOL);
        if (pb == NULL) {
            restore_linkoutputs();
            *arp_result  = -1;
            *icmp_result = -1;
            return;
        }
        pbuf_take(pb, arp_frame, ARP_FRAME_LEN);
        ethernet_input(pb, input_netif);          /* ownership transferred */
    }

    *arp_result = s_arp_reply_seen;

    /* ============================================================
     *  ADIM 2 — ICMP Echo Request:  Ping paketi
     *
     *  Gercek hayat: PC, ARP basarili olduktan sonra bunu gonderir.
     *  ip_input() → icmp_input() → ip_output_if() → etharp_output()
     *  → netif->linkoutput (bizim hook).
     *
     *  Eger Adim 1 basarisiz olduysa (ARP Reply yok), board
     *  cevap MAC'ini bilemez → ICMP Reply linkoutput'a ulasamaz
     *  → s_icmp_reply_seen = 0 kalir.
     *  Bu, gercek hayattaki davranisla birebir aynidir.
     * ============================================================ */
    {
        uint8_t ping_frame[PING_FRAME_LEN];
        s_icmp_reply_seen = 0;

        build_ping_frame(ping_frame, params);

        pb = pbuf_alloc(PBUF_RAW, PING_FRAME_LEN, PBUF_POOL);
        if (pb == NULL) {
            restore_linkoutputs();
            *icmp_result = -1;
            return;
        }
        pbuf_take(pb, ping_frame, PING_FRAME_LEN);
        ethernet_input(pb, input_netif);          /* ownership transferred */
    }

    *icmp_result = s_icmp_reply_seen;

    /* ---- Restore ---- */
    restore_linkoutputs();
}

/* ===================================================================== */
/*  PUBLIC: DIAGNOSTIC SUITE                                              */
/* ===================================================================== */

static void print_test_row(const char *label,
                           int arp, int arp_exp,
                           int icmp, int icmp_exp)
{
    sciDisplayText(sciREG1, (uint8_t*)"  ", 2);
    sciDisplayText(sciREG1, (uint8_t*)label, (uint32_t)strlen(label));
    UART_STR("\r\n");

    UART_STR("    ARP Reply:  ");
    if (arp == 1)       UART_STR("EVET ");
    else if (arp == 0)  UART_STR("HAYIR");
    else                UART_STR("HATA ");
    if (arp == arp_exp) UART_STR(" [PASS]\r\n");
    else                UART_STR(" [FAIL]\r\n");

    UART_STR("    ICMP Reply: ");
    if (icmp == 1)       UART_STR("EVET ");
    else if (icmp == 0)  UART_STR("HAYIR");
    else                 UART_STR("HATA ");
    if (icmp == icmp_exp) UART_STR(" [PASS]\r\n");
    else                  UART_STR(" [FAIL]\r\n");
}

void run_ping_diagnostics(void)
{
    struct netif        *nif;
    ping_test_params_t   p;
    int                  ar, ir;   /* arp result, icmp result */
    uint8_t              fake_mac[6]  = {0x11,0x22,0x33,0x44,0x55,0x66};
    uint8_t              wrong_mac[6] = {0xAA,0xBB,0xCC,0xDD,0xEE,0xFF};

    nif = netif_default;
    if (nif == NULL) nif = netif_list;
    if (nif == NULL) {
        UART_STR("\r\n[DIAG] HATA: netif bulunamadi!\r\n");
        return;
    }

    UART_STR("\r\n========== PING DIAGNOSTICS ==========\r\n");

    /* ---- Test 1: Dogru MAC + Ana IP ---- */
    memcpy(p.dst_mac, nif->hwaddr, 6);
    memcpy(p.src_mac, fake_mac, 6);
    p.src_ip[0]=192; p.src_ip[1]=168;
    p.src_ip[2]=2;   p.src_ip[3]=100;
    get_ip_bytes(&nif->ip_addr, p.dst_ip);

    inject_realistic_ping_test(&p, nif, &ar, &ir);
    print_test_row("Test1 [Dogru MAC + Ana IP]",
                   ar, 1,    /* ARP: beklenen EVET */
                   ir, 1);   /* ICMP: beklenen EVET */

    /* ---- Test 2: Dogru MAC + Yanlis IP ---- */
    p.dst_ip[0]=10; p.dst_ip[1]=0;
    p.dst_ip[2]=0;  p.dst_ip[3]=1;

    inject_realistic_ping_test(&p, nif, &ar, &ir);
    print_test_row("Test2 [Dogru MAC + Yanlis IP]",
                   ar, 0,    /* ARP: beklenen HAYIR */
                   ir, 0);   /* ICMP: beklenen HAYIR */

    /* ---- Test 3: Yanlis MAC + Dogru IP ---- */
    memcpy(p.dst_mac, wrong_mac, 6);
    get_ip_bytes(&nif->ip_addr, p.dst_ip);

    inject_realistic_ping_test(&p, nif, &ar, &ir);
    print_test_row("Test3 [Yanlis MAC + Dogru IP]",
                   ar, 1,    /* ARP: beklenen EVET (broadcast) */
                   ir, 1);   /* ICMP: beklenen EVET (MAC=SW'de kontrol yok) */

    /* ---- Test 4: Dogru MAC + Alias IP ---- */
    {
        struct netif *alias = NULL;
        struct netif *it;
        for (it = netif_list; it != NULL; it = it->next) {
            if (it != nif) { alias = it; break; }
        }

        if (alias != NULL) {
            memcpy(p.dst_mac, nif->hwaddr, 6);
            get_ip_bytes(&alias->ip_addr, p.dst_ip);

            inject_realistic_ping_test(&p, nif, &ar, &ir);
            print_test_row("Test4 [Dogru MAC + Alias IP]",
                           ar, 1,    /* ARP: beklenen EVET (ama muhtemelen HAYIR!) */
                           ir, 1);   /* ICMP: beklenen EVET (ama ARP'siz olmaz) */
        } else {
            UART_STR("  Test4 -- ATLANDI (alias netif yok)\r\n");
        }
    }

    UART_STR("========================================\r\n");
    UART_STR("Test4 ARP=HAYIR ise: etharp_arp_input()\r\n");
    UART_STR("sadece birincil netif IP'sini kontrol\r\n");
    UART_STR("eder, alias IP icin ARP cevabi vermez.\r\n");
    UART_STR("Bu sorunun cozumu etharp.c yamasidir.\r\n");
}

/* ===================================================================== */
/*  PUBLIC: TEK PING TESTI (kolay kullanim)                               */
/* ===================================================================== */

void inject_fake_ping_packet(void)
{
    struct netif       *nif;
    ping_test_params_t  p;
    int                 ar, ir;

    /* ================================================================
     *  BURADAN DEGISTIRIN
     * ================================================================ */

    /* HEDEF MAC — Kartin kendi MAC'ini kullanmak icin: USE_BOARD_MAC=1
     *             Elle girmek icin:                    USE_BOARD_MAC=0 */
    #define USE_BOARD_MAC  1
    uint8_t hedef_mac[6] = { 0x00, 0x08, 0xEE, 0x03, 0xA6, 0x6C };

    /* HEDEF IP — Kartin kendi IP'sini kullanmak icin:  USE_BOARD_IP=1
     *            Elle girmek icin:                     USE_BOARD_IP=0
     * !! PAKETIN KABUL/RED EDILMESINI BU ALAN BELIRLER !! */
    #define USE_BOARD_IP   0
    uint8_t hedef_ip[4] = { 192, 168, 2, 50 };

    /* KAYNAK MAC — Sahte gonderici MAC */
    uint8_t kaynak_mac[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };

    /* KAYNAK IP  — Sahte gonderici IP */
    uint8_t kaynak_ip[4] = { 192, 168, 2, 100 };

    /* ================================================================ */

    nif = netif_default;
    if (nif == NULL) nif = netif_list;
    if (nif == NULL) {
        UART_STR("\r\n[TEST] HATA: Aktif netif yok!\r\n");
        return;
    }

    UART_STR("\r\n[TEST] Gercekci Ping Testi Baslatiliyor...\r\n");

    /* Hedef MAC */
    #if USE_BOARD_MAC
        memcpy(p.dst_mac, nif->hwaddr, 6);
    #else
        memcpy(p.dst_mac, hedef_mac, 6);
    #endif

    /* Hedef IP */
    #if USE_BOARD_IP
        get_ip_bytes(&nif->ip_addr, p.dst_ip);
    #else
        memcpy(p.dst_ip, hedef_ip, 4);
    #endif

    /* Kaynak */
    memcpy(p.src_mac, kaynak_mac, 6);
    memcpy(p.src_ip, kaynak_ip, 4);

    /* Testi calistir */
    inject_realistic_ping_test(&p, nif, &ar, &ir);

    /* Sonuclari yazdir */
    UART_STR("\r\n");
    UART_STR("  Adim 1 - ARP Reply:       ");
    if (ar == 1)       UART_STR("EVET  (Board MAC adresini bildirdi)\r\n");
    else if (ar == 0)  UART_STR("HAYIR (Board bu IP icin ARP cevabi vermedi)\r\n");
    else               UART_STR("HATA  (pbuf tahsis edilemedi)\r\n");

    UART_STR("  Adim 2 - ICMP Echo Reply: ");
    if (ir == 1)       UART_STR("EVET  (Ping cevabi uretildi)\r\n");
    else if (ir == 0)  UART_STR("HAYIR (Ping cevabi uretilemedi)\r\n");
    else               UART_STR("HATA  (pbuf tahsis edilemedi)\r\n");

    UART_STR("\r\n");

    /* Tani koymaya yardimci mesajlar */
    if (ar == 1 && ir == 1) {
        UART_STR("[SONUC] Her iki adim da basarili.\r\n");
        UART_STR("        Gercek PC'den ping calismali.\r\n");
    }
    else if (ar == 0 && ir == 0) {
        UART_STR("[SONUC] ARP cevabi yok! PC bu IP'nin MAC'ini\r\n");
        UART_STR("        cozemez -> ping paketi gonderilemez.\r\n");
        UART_STR("        COZUM: etharp.c'de netif_list taramasi\r\n");
        UART_STR("        eklenmelidir.\r\n");
    }
    else if (ar == 0 && ir == 1) {
        UART_STR("[SONUC] ARP yok ama ICMP var! ETHARP_TRUST_IP_MAC\r\n");
        UART_STR("        sayesinde calisti. Gercek PC'de yine de\r\n");
        UART_STR("        ARP cozulemez -> ping calismaz.\r\n");
        UART_STR("        COZUM: etharp.c'de netif_list taramasi\r\n");
        UART_STR("        eklenmelidir.\r\n");
    }
    else if (ar == 1 && ir == 0) {
        UART_STR("[SONUC] ARP tamam ama ICMP yok! ip_input veya\r\n");
        UART_STR("        icmp_input asamasinda sorun var.\r\n");
    }

    #undef USE_BOARD_MAC
    #undef USE_BOARD_IP
}
