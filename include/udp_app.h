/**
 * @file udp_app.h
 * @brief Basit UDP uygulamasi — RAW API, bare-metal (NO_SYS=1).
 *
 *   Tek PCB ile hem ana IP hem alias IP uzerinden UDP paket
 *   alip gondermeyi saglar. RX callback ISR baglaminda calisir,
 *   veri main loop'a poll ile aktarilir.
 *
 *  Created on: 14 Tem 2026
 *      Author: doruk_yaman
 */

#ifndef UDP_APP_H_
#define UDP_APP_H_

#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ===================================================================== */
/*  CONFIGURATION                                                        */
/* ===================================================================== */

/** Tek RX mesaj slotundaki maksimum veri boyutu (byte).
 *  Daha buyuk paketler bu boyuta kirpilir. */
#define UDP_APP_RX_BUF_SIZE   512

/* ===================================================================== */
/*  TYPES                                                                */
/* ===================================================================== */

/**
 * @brief ISR'dan main loop'a aktarilan RX paket bilgisi.
 *
 * ISR callback gelen paketi bu yapiya kopyalar;
 * main loop udp_app_poll_rx() ile okur.
 */
typedef struct {
    ip_addr_t  src_ip;                    /**< Gonderenin IP adresi          */
    u16_t      src_port;                  /**< Gonderenin port numarasi      */
    u16_t      data_len;                  /**< Veri uzunlugu (byte)          */
    uint8_t    data[UDP_APP_RX_BUF_SIZE]; /**< Paket verisi (payload)        */
    uint8_t    valid;                     /**< 1 = okunacak veri var          */
} udp_app_rx_msg_t;

/* ===================================================================== */
/*  PUBLIC API                                                            */
/* ===================================================================== */

/**
 * @brief UDP PCB olusturur, IP_ADDR_ANY:listen_port'a bind eder,
 *        receive callback kaydeder.
 *
 * IP_ADDR_ANY kullanildigi icin hem ana IP (192.168.2.44) hem de
 * alias IP (192.168.2.50) uzerinden gelen paketler tek PCB ile
 * yakalanir.
 *
 * @param listen_port  Dinlenecek UDP port numarasi
 * @return ERR_OK basari, ERR_MEM (PCB havuzu dolu), ERR_USE (port mesgul)
 */
err_t udp_app_init(u16_t listen_port);

/**
 * @brief Belirtilen hedefe UDP paketi gonderir.
 *
 * PCB IP_ADDR_ANY'ye bagli oldugundan, ip_route(dest_ip) ile
 * cikis netif'i bulunur ve o netif'in IP'si kaynak adres olur
 * (genelde birincil IP: 192.168.2.44).
 *
 * @param dest_ip    Hedef IP adresi
 * @param dest_port  Hedef port numarasi
 * @param data       Gonderilecek veri buffer'i
 * @param len        Veri uzunlugu (byte)
 * @return ERR_OK basari, ERR_MEM (pbuf allocation hatasi),
 *         ERR_CONN (init yapilmamis), ERR_RTE (route bulunamadi)
 */
err_t udp_app_send(ip_addr_t *dest_ip, u16_t dest_port,
                   const uint8_t *data, u16_t len);

/**
 * @brief ISR callback'inin doldurdugu RX mesajini main loop'a verir.
 *
 * Non-blocking: mesaj yoksa 0 dondurur.
 * Mesaj varsa msg yapisina kopyalar ve dahili slotu serbest birakir.
 * ISR <-> main senkronizasyonu SYS_ARCH_PROTECT ile saglanir.
 *
 * @param msg  [out] Kopyalanacak mesaj yapisi
 * @return 1 = yeni mesaj var (msg dolduruldu), 0 = mesaj yok
 */
uint8_t udp_app_poll_rx(udp_app_rx_msg_t *msg);

/**
 * @brief UDP PCB'yi kaldirir ve kaynaklari serbest birakir.
 */
void udp_app_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* UDP_APP_H_ */
