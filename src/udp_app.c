/**
 * @file udp_app.c
 * @brief Basit UDP uygulamasi — RAW API, bare-metal (NO_SYS=1).
 *
 *   RX: ISR baglaminda calisan callback, gelen veriyi statik bir
 *       mesaj yapisina kopyalar. Main loop udp_app_poll_rx() ile okur.
 *   TX: udp_app_send() ile pbuf_alloc + udp_sendto.
 *
 *   Mimari Notlar:
 *   - Receive callback EMACCore0RxIsr zincirinden cagirilir (ISR).
 *   - Callback icinde busy-wait I/O (sciSend vb.) YASAKTIR.
 *   - ISR <-> main veri aktarimi volatile struct + SYS_ARCH_PROTECT ile
 *     saglanir.
 *   - Tek mesaj slotu kullanilir (basitlik). Slot dolu iken gelen
 *     yeni paketler DROP edilir.
 *
 *  Created on: 14 Tem 2026
 *      Author: doruk_yaman
 */

#include "udp_app.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"        /* SYS_ARCH_PROTECT / UNPROTECT */
#include <string.h>

/* ===================================================================== */
/*  PRIVATE STATE                                                        */
/* ===================================================================== */

/** UDP PCB — modul omru boyunca tek instance */
static struct udp_pcb *s_pcb = NULL;

/** ISR -> main loop mesaj slotu.
 *  volatile: ISR ile main arasinda paylasilir.
 *  Tek slot = producer-consumer, ISR yazar / main okur. */
static volatile udp_app_rx_msg_t s_rx_msg;

/** Basarili RX sayaci (debug/diagnostik icin) */
static volatile uint32_t s_rx_count = 0;

/** Drop edilen paket sayaci (slot dolu iken gelen paketler) */
static volatile uint32_t s_rx_drop_count = 0;

/* ===================================================================== */
/*  ISR-SAFE RX CALLBACK                                                 */
/* ===================================================================== */

/**
 * @brief lwIP udp_recv callback'i — EMACCore0RxIsr zincirinden cagirilir.
 *
 * KURALLAR:
 *   1. pbuf_free(p) ZORUNLU — lwIP otomatik yapmaz.
 *   2. Busy-wait I/O (sciSend, sciDisplayText) YASAK.
 *   3. Mumkun olan en kisa surede cikilmali.
 *
 * ONEMLI: 'addr' parametresi pbuf'un icerisine isaret eder.
 *         pbuf_free(p) cagrildiktan sonra addr gecersiz olur!
 *         Bu yuzden addr'i pbuf_free'den ONCE kopyaliyoruz.
 *
 * @param arg   Kullanici arguemani (NULL — udp_recv'de boyle ayarladik)
 * @param pcb   Bu paketi alan UDP PCB
 * @param p     Gelen paket verisi (zincirli pbuf olabilir)
 * @param addr  Gonderenin IP adresi (!! pbuf icerisine isaret eder !!)
 * @param port  Gonderenin port numarasi (host byte order)
 */
static void udp_app_recv_cb(void *arg,
                            struct udp_pcb *pcb,
                            struct pbuf *p,
                            ip_addr_t *addr,
                            u16_t port)
{
    /* Kullanilmayan parametreler — derleyici uyarisini engelle */
    (void)arg;
    (void)pcb;

    /* NULL pbuf = uzak taraf "kapatti" sinyali (UDP'de nadir ama mumkun) */
    if (p == NULL) {
        return;
    }

    /*
     * Tek mesaj slotu stratejisi:
     *   valid == 0 → slot bos, yeni veri yazilabilir
     *   valid == 1 → slot dolu (main henuz okumamis), paketi DROP et
     *
     * Neden ring buffer degil?
     *   - Basitlik: ISR icinde minimum islem
     *   - Cogu kullanim senaryosunda main loop yeterince hizli okur
     *   - Uretim kodunda ring buffer'a gecis kolay (ayni arayuz)
     */
    if (s_rx_msg.valid == 0) {
        u16_t copy_len = p->tot_len;
        if (copy_len > UDP_APP_RX_BUF_SIZE) {
            copy_len = UDP_APP_RX_BUF_SIZE;   /* Kirp, drop etme */
        }

        /* 1. addr'i ONCE kopyala (pbuf_free sonrasi gecersiz olacak!) */
        ip_addr_set((ip_addr_t *)&s_rx_msg.src_ip, addr);
        s_rx_msg.src_port = port;
        s_rx_msg.data_len = copy_len;

        /* 2. Zincirli pbuf'lardan duz diziye kopyala.
         *    pbuf_copy_partial() chain'deki tum parcalari birlestirerek
         *    hedef buffer'a yazar — tek bir memcpy'den guvenlidir. */
        pbuf_copy_partial(p, (void *)s_rx_msg.data, copy_len, 0);

        /* 3. Son olarak valid flag'ini set et.
         *    Sira ONEMLI: once veri, sonra flag — main loop flag'i
         *    gordugunde verinin tamamlanmis oldugu garanti edilir. */
        s_rx_msg.valid = 1;

        s_rx_count++;
    } else {
        /* Slot dolu — bu paketi DROP ediyoruz */
        s_rx_drop_count++;
    }

    /* ZORUNLU: pbuf'u serbest birak.
     * Bunu yapmazsak pbuf havuzu tukenene kadar memory leak olusur,
     * sonra yeni paketler alınamaz (PBUF_POOL_SIZE exhaustion). */
    pbuf_free(p);
}

/* ===================================================================== */
/*  PUBLIC API                                                            */
/* ===================================================================== */

/**
 * @brief UDP PCB olusturur, IP_ADDR_ANY:listen_port'a bind eder,
 *        receive callback kaydeder.
 *
 * Cagri sirasi (lwip_main.c icinde):
 *   lwIPCoreInit() -> lwIPNetifAdd() -> lwIPAliasAdd() -> udp_app_init()
 *
 * IP_ADDR_ANY ile bind etmek, udp_input() icindeki PCB eslestirme
 * mantiginda su anlama gelir:
 *   - pcb->local_port == hedef port  → eslesiyor
 *   - pcb->local_ip == 0.0.0.0       → herhangi bir hedef IP kabul
 * Dolayisiyla hem 192.168.2.44 hem 192.168.2.50'ye gelen paketler
 * TEK PCB ile yakalanir.
 */
err_t udp_app_init(u16_t listen_port)
{
    err_t err;

    /* Onceki PCB varsa temizle (yeniden init senaryosu) */
    if (s_pcb != NULL) {
        udp_remove(s_pcb);
        s_pcb = NULL;
    }

    /* Mesaj slotunu temizle */
    s_rx_msg.valid = 0;
    s_rx_count = 0;
    s_rx_drop_count = 0;

    /* 1. Yeni UDP PCB olustur.
     *    memp havuzundan (MEMP_UDP_PCB) bir slot alir.
     *    MEMP_NUM_UDP_PCB default=4, DHCP 1 adet kullaniyor,
     *    bu yuzden en az 2 bos slot var. */
    s_pcb = udp_new();
    if (s_pcb == NULL) {
        return ERR_MEM;   /* PCB havuzu dolu */
    }

    /* 2. IP_ADDR_ANY:listen_port'a bind et.
     *    IP_ADDR_ANY = 0.0.0.0 → tum yerel IP'lere gelen paketleri kabul et.
     *    Bu hem ana hem alias IP icin calisan en basit yaklasim. */
    err = udp_bind(s_pcb, IP_ADDR_ANY, listen_port);
    if (err != ERR_OK) {
        udp_remove(s_pcb);
        s_pcb = NULL;
        return err;       /* Muhtemelen ERR_USE (port zaten bagli) */
    }

    /* 3. Receive callback kaydet.
     *    Bundan sonra listen_port'a gelen her UDP paketi icin
     *    udp_app_recv_cb() cagirilacak (ISR baglaminda!). */
    udp_recv(s_pcb, udp_app_recv_cb, NULL);

    return ERR_OK;
}

/**
 * @brief Belirtilen hedefe UDP paketi gonderir.
 *
 * Islem adimlari:
 *   1. pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM)
 *      → PBUF_TRANSPORT: lwIP UDP+IP header'lari icin bosta yer birakir
 *      → PBUF_RAM: heap'ten tahsis (TX icin uygun, pbuf havuzunu tuketmez)
 *   2. memcpy ile veriyi pbuf payload'ina kopyala
 *   3. udp_sendto(pcb, p, dst_ip, dst_port)
 *      → ip_route(dst_ip) ile cikis netif'ini bulur
 *      → PCB IP_ADDR_ANY'ye bagli oldugu icin netif->ip_addr kaynak IP olur
 *      → UDP header + IP header + Ethernet header ekler
 *      → hdkif_transmit() ile EMAC'a verir
 *   4. pbuf_free(p) — gonderme tamamlandi, pbuf'u serbest birak
 *
 * NOT: Kaynak IP her zaman ip_route() sonucuna gore belirlenir.
 * Ayni subnet'te bir hedefe gonderirken genelde birincil netif
 * (192.168.2.44) secilir. Alias IP'den (192.168.2.50) gondermek
 * icin udp_sendto_if() kullanilmali — bu fonksiyon bunu YAPMAZ.
 */
err_t udp_app_send(ip_addr_t *dest_ip, u16_t dest_port,
                   const uint8_t *data, u16_t len)
{
    struct pbuf *p;
    err_t err;

    if (s_pcb == NULL) {
        return ERR_CONN;  /* udp_app_init() henuz cagrilmamis */
    }

    if (data == NULL || len == 0) {
        return ERR_ARG;
    }

    /* PBUF_TRANSPORT → pbuf'un basinda UDP (8 byte) + IP (20 byte)
     * header'lari icin yer ayrilir. Boylece udp_sendto_if icinde
     * pbuf_header() cagrildiginda yeni pbuf tahsisi gerekmez.
     *
     * PBUF_RAM → MEM_SIZE (30K) heap'inden tahsis edilir.
     * PBUF_POOL degil, cunku TX paketleri icin pool'u tuketmek
     * istemiyoruz (pool RX icin gerekli). */
    p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) {
        return ERR_MEM;   /* Heap dolu veya cok buyuk paket */
    }

    /* Veriyi pbuf payload alanina kopyala */
    memcpy(p->payload, data, len);

    /* Gonderi — udp_sendto icsel islem sirasi:
     *   1. ip_route(dest_ip) → netif bul (subnet eslesmesi)
     *   2. udp_sendto_if(pcb, p, dest_ip, dest_port, netif) cagirir
     *   3. UDP header ekler (src_port, dst_port, checksum)
     *   4. ip_output_if() → IP header ekler
     *   5. netif->output() → etharp_output() → ARP cozumleme
     *   6. netif->linkoutput() → hdkif_transmit() → EMAC DMA */
    err = udp_sendto(s_pcb, p, dest_ip, dest_port);

    /* ZORUNLU: pbuf'u serbest birak.
     * udp_sendto basarili olsa bile pbuf'u otomatik serbest birakmaz.
     * (lwIP tasarim karari: arayan pbuf'un sahibidir.) */
    pbuf_free(p);

    return err;
}

/**
 * @brief ISR callback'inin doldurdugu RX mesajini main loop'a verir.
 *
 * Bu fonksiyon NON-BLOCKING'dir: mesaj yoksa hemen 0 dondurur.
 * while(1) loop'unda her iterasyonda bir kez cagrilmali.
 *
 * ISR ile main arasindaki race condition'i onlemek icin
 * SYS_ARCH_PROTECT/UNPROTECT kullanilir. Bu makrolar
 * ARM Cortex-R'da interrupt'lari gecici olarak kapatir.
 */
uint8_t udp_app_poll_rx(udp_app_rx_msg_t *msg)
{
    SYS_ARCH_DECL_PROTECT(old_level);

    if (msg == NULL) {
        return 0;
    }

    /* Hizli kontrol — interrupt kapatmadan once oku.
     * volatile oldugu icin derleyici optimize edemez. */
    if (s_rx_msg.valid == 0) {
        return 0;
    }

    /* Kritik bolge: ISR'nin struct'i yarim yazmasi / uzerine
     * yazmasini engelle. ARM'da bu _disable_IRQ() / _enable_IRQ()
     * ile gerceklenir (lwipopts.h'de SYS_LIGHTWEIGHT_PROT=1). */
    SYS_ARCH_PROTECT(old_level);

    memcpy(msg, (const void *)&s_rx_msg, sizeof(udp_app_rx_msg_t));
    s_rx_msg.valid = 0;   /* Slotu serbest birak — ISR yeni veri yazabilir */

    SYS_ARCH_UNPROTECT(old_level);

    return 1;
}

/**
 * @brief UDP PCB'yi kaldirir ve kaynaklari serbest birakir.
 *
 * Cagri sonrasi:
 *   - PCB udp_pcbs listesinden cikarilir
 *   - MEMP_UDP_PCB slotu serbest kalir
 *   - Artik o porta gelen paketler icin callback cagirilmaz
 */
void udp_app_deinit(void)
{
    if (s_pcb != NULL) {
        udp_remove(s_pcb);
        s_pcb = NULL;
    }
    s_rx_msg.valid = 0;
}
