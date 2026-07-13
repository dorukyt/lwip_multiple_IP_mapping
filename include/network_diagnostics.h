/*
 * network_diagnostics.h
 *
 * Realistic ICMP ping diagnostics for TMS570LC4357 + lwIP 1.4.1
 *
 * Gercek bir "ping" isleminde paket su asamalardan gecer:
 *
 *   PC                                Board
 *    |                                  |
 *    |--- ARP Request: "IP kimin?" ---->|  Adim 1
 *    |<-- ARP Reply:   "MAC benim" ----|  Adim 2
 *    |                                  |
 *    |--- ICMP Echo Request ----------->|  Adim 3
 *    |<-- ICMP Echo Reply --------------|  Adim 4
 *
 * Bu modul her iki asamayi da (ARP + ICMP) test eder ve
 * hangi adimda basarisiz oldugunu raporlar.
 *
 *  Created on: 10 Tem 2026
 *      Author: doruk_yaman
 */

#ifndef INCLUDE_NETWORK_DIAGNOSTICS_H_
#define INCLUDE_NETWORK_DIAGNOSTICS_H_

#include <stdint.h>
#include "lwip/netif.h"

/* ===================================================================== */
/*  TYPES                                                                 */
/* ===================================================================== */

/**
 * @brief Test parametreleri — tum adresler network byte order (MSB first).
 */
typedef struct {
    uint8_t dst_mac[6];     /**< Hedef MAC  (kartin MAC'i olmali)       */
    uint8_t src_mac[6];     /**< Kaynak MAC (sahte gonderici / PC)      */
    uint8_t src_ip[4];      /**< Kaynak IP  (sahte gonderici / PC)      */
    uint8_t dst_ip[4];      /**< Hedef IP   (test edilecek IP adresi)   */
} ping_test_params_t;

/* ===================================================================== */
/*  PUBLIC API                                                            */
/* ===================================================================== */

/**
 * @brief Gercekci ping testi — ARP + ICMP asamalarini ayri ayri test eder.
 *
 * Adim 1: ARP Request enjekte eder ("dst_ip kimin?")
 *         → Board ARP Reply gonderdiyse arp_result = 1
 *
 * Adim 2: ICMP Echo Request enjekte eder (dst_ip'ye ping)
 *         → Board ICMP Echo Reply gonderdiyse icmp_result = 1
 *
 * Eger arp_result=0 ise gercek hayatta da ping calismaz:
 * PC hedef MAC'i cozemedigi icin paketi gonderemez.
 *
 * @param params       Kaynak/hedef MAC ve IP adresleri
 * @param input_netif  Paketin enjekte edilecegi arayuz (netif_default)
 * @param arp_result   [out]  1=ARP Reply uretildi, 0=uretilmedi, -1=hata
 * @param icmp_result  [out]  1=ICMP Reply uretildi, 0=uretilmedi, -1=hata
 */
void inject_realistic_ping_test(const ping_test_params_t *params,
                                struct netif *input_netif,
                                int *arp_result,
                                int *icmp_result);

/**
 * @brief 4 farkli senaryo ile tam diagnostik suiti calistirir.
 *        Sonuclari UART (sciREG1) uzerinden yazdirir.
 */
void run_ping_diagnostics(void);

/**
 * @brief Tek ping testi (ana fonksiyon).
 *        ARP ve ICMP sonuclarini ayri ayri raporlar.
 */
void inject_fake_ping_packet(void);

#endif /* INCLUDE_NETWORK_DIAGNOSTICS_H_ */
