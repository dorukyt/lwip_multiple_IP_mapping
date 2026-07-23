/** @file lwip_main.c
 *   @brief Application main file (dual static IP / IP aliasing versiyonu)
 *
 *   Bu dosya, orijinal lwip_main.c'nin EMAC_LwIP_Main() fonksiyonunu
 *   TI'nin tek-parca lwIPInit() cagrisi yerine, lwiplib.c'ye eklenen
 *   modular API (lwIPCoreInit / lwIPNetifAdd / lwIPAliasAdd) ile
 *   yeniden yazilmis halidir. SCI/log, httpd, ISR ve IOMM mux
 *   fonksiyonlari AYNEN korunmustur.
 */

#if defined(_TMS570LC43x_) || defined(_RM57Lx_)
#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_emac.h"
#include "HL_mdio.h"
#include "HL_phy_dp83640.h"
#include "HL_sci.h"
#else
#include "sys_common.h"
#include "system.h"
#include "emac.h"
#include "mdio.h"
#include "phy_dp83640.h"
#include "sci.h"
#endif

#include "lwipopts.h"
#include "lwiplib.h"
#include "httpd.h"
#include "lwip\inet.h"
#include "locator.h"

#include "network_diagnostics.h"
#include "UDP_source.h"
#include "terminal_interface.h"
#include <string.h>
#include <stdio.h>

#define TMS570_MDIO_BASE_ADDR 	0xFCF78900u
#define TMS570_EMAC_BASE_ADDR	0xFCF78000u
#define DPS83640_PHYID			0x20005CE1u
#define PHY_ADDR				1

#if defined(_TMS570LC43x_) || defined(_RM57Lx_)
#define sciREGx	sciREG1
#else
#define sciREGx	scilinREG
#endif

uint8_t		txtCRLF[]			= {'\r', '\n'};
uint8_t  	txtTitle[] 			= {"HERCULES MICROCONTROLLERS"};
uint8_t		txtTI[]				= {"Texas Instruments"};
uint8_t		txtLittleEndian[] 	= {"Little Endian device"};
uint8_t		txtBigEndian[]		= {"Big Endian device"};
uint8_t		txtEnetInit[]		= {"Initializing ethernet (dual static IP)"};
uint8_t		txtIPAddrTxt[]		= {"Ana IP Address:   "};
uint8_t		txtIPAddrTxt2[]		= {"Sanal IP Address: "};
uint8_t     txtIPAddrTxt3[]     = {"Alici IP Address: "};
uint8_t     txtIPAddrTxt3_IP[]     = {"192.168.2.100"};
uint8_t		txtNote1[]			= {"Webserver accessible @ http:\\\\"};
uint8_t		txtErrorInit[]		= {"-------- ERROR INITIALIZING HARDWARE --------"};
uint8_t		 * txtIPAddrItoA;
uint8_t		 * txtIPAddrItoA2;

void 	iommUnlock			(void);
void 	iommLock			(void);
void 	iommMuxEnableMdio	(void);
void 	iommMuxEnableRmii	(void);
void 	iommMuxEnableMii	(void);
void 	IntMasterIRQEnable	(void);
void 	smallDelay			(void);
void 	sciDisplayText		(sciBASE_t *sci, uint8_t *text,uint32_t length);

extern volatile uint8_t send_main_flag;
extern volatile uint8_t send_alias_flag;

void smallDelay(void) {
	  static volatile unsigned int delayval;
	  delayval = 10000;
	  while(delayval--);
}

void EMAC_LwIP_Main (uint8_t * macAddress)
{
    //uint8_t 		testChar;
    struct in_addr 	devIPAddress;
    struct in_addr 	devIPAddress2;
    unsigned int	anaIpAddr;
    unsigned int	sanalIpAddr;
    char str_anaIp[16]   = {0};
    char str_sanalIp[16] = {0};

    ip_addr_t temp_ip;
    /* --- Statik IP tanimlari ---
     * ip_ana : cihazin birincil (default route'un ciktigi) adresi
     * ip_sanal: ayni fiziksel EMAC + ayni MAC uzerinde ikinci (alias) IP
     */
    uint8_t ip_ana[4]      = { 10, 0, 0, 10  };
    uint8_t netmask_ana[4] = { 255, 255, 255, 0 };
    uint8_t gateway_ana[4] = { 10, 0, 0, 1 };

    uint8_t ip_sanal[4]      = { 11, 0, 0, 11 };
    uint8_t netmask_sanal[4] = { 255, 255, 255, 0 };
    uint8_t gateway_sanal[4] = { 11, 0, 0, 1 };

    //TODO:
    //Create a linked list design for destination IP addresses to be later used for
    //selecting, adding, deleting and changing the dest addresses
    ip_addr_t dest;
    IP4_ADDR(&dest, 12, 0, 0, 100);
    const char *main_message = "Hello from 10.0.0.10";
    const char *alias_message = "Hello from 11.0.0.11";

	sciInit();

	IntMasterIRQEnable();
	_enable_FIQ();

	sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
	sciDisplayText(sciREGx, txtTitle, sizeof(txtTitle));
	sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
	sciDisplayText(sciREGx, txtTI, sizeof(txtTI));
	sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
#ifdef __little_endian__        
    sciDisplayText(sciREGx, txtLittleEndian, sizeof(txtLittleEndian));
    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
#else        
    sciDisplayText(sciREGx, txtBigEndian, sizeof(txtBigEndian));
    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
#endif

	sciDisplayText(sciREGx, txtEnetInit, sizeof(txtEnetInit));

	/* 1) Donanim + lwIP core: SADECE BIR KERE.
	 *    hdkif_macaddrset() burada calisir. */
	if (0 == lwIPCoreInit(0, macAddress))
	{
		sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
		sciDisplayText(sciREGx, txtErrorInit, sizeof(txtErrorInit));
		sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
		return;
	}

	/* 2) Birincil (fiziksel) IP: hdkif_init() -> PHY reset/autoneg
	 *    SADECE bu cagrida tetiklenir. */
	anaIpAddr = lwIPNetifAdd(0,
		*((uint32_t *)ip_ana),
		*((uint32_t *)netmask_ana),
		*((uint32_t *)gateway_ana),
		1 /* default route */);

	if (0 == anaIpAddr)
	{
		sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
		sciDisplayText(sciREGx, txtErrorInit, sizeof(txtErrorInit));
		sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
		return;
	}

	/* 3) Sanal (alias) IP: donanima dokunmaz, sadece netif_list'e
	 *    yeni bir dugum ekler; TX cikisi birincil netif ile paylasilir. */
	sanalIpAddr = lwIPAliasAdd(0,
		*((uint32_t *)ip_sanal),
		*((uint32_t *)netmask_sanal),
		*((uint32_t *)gateway_sanal));

	if (0 == sanalIpAddr)
	{
		sciDisplayText(sciREGx, (uint8_t*)"UYARI: Sanal IP eklenemedi, ana IP ile devam ediliyor", 54);
		sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
	}

	struct netif *g_main_netif  = lwIPNetifPtrGet(0);
	struct netif *g_alias_netif = lwIPAliasNetifPtrGet(0);

	sciDisplayText(sciREGx, (uint8_t*)"..DONE", sizeof("..DONE"));
	sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));

	devIPAddress.s_addr = anaIpAddr;
	txtIPAddrItoA = (uint8_t *)inet_ntoa(devIPAddress);

	if (sanalIpAddr != 0)
	{
		devIPAddress2.s_addr = sanalIpAddr;
		txtIPAddrItoA2 = (uint8_t *)inet_ntoa(devIPAddress2);
	}

	LocatorConfig(macAddress, "HDK enet_lwip (dual-ip)");

	//sciDisplayText(sciREGx, (uint8_t*)"Starting Web Server", sizeof("Starting Web Server"));
	//httpd_init();
	sciDisplayText(sciREGx, (uint8_t*)"..DONE", sizeof("..DONE"));
	sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));

	u8_t h_main;
	u8_t h_alias;

	if (udp_source_add_listener(&g_main_netif->ip_addr, 5000, &h_main) == ERR_OK)
    {
        char ip_str[16];
        ipaddr_ntoa_r(&g_main_netif->ip_addr, ip_str, sizeof(ip_str));

        char msg[64];
        int msg_len = snprintf(msg, sizeof(msg),
                               "\r\nUDP Source '%s' init OK (port 5000)",
                               ip_str);
        sciDisplayText(sciREGx, (uint8_t*) msg, (uint32_t) msg_len);
        sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
    }
    else
    {
        sciDisplayText(sciREGx, (uint8_t*) "\r\nUDP App init FAILED!", 22);
        sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
    }

    if (udp_source_add_listener(&g_alias_netif->ip_addr, 5000, &h_alias) == ERR_OK)
    {
        char ip_str[16];
        ipaddr_ntoa_r(&g_alias_netif->ip_addr, ip_str, sizeof(ip_str));

        char msg[64];
        int msg_len = snprintf(msg, sizeof(msg),
                               "\r\nUDP Source '%s' init OK (port 5000)",
                               ip_str);
        sciDisplayText(sciREGx, (uint8_t*) msg, (uint32_t) msg_len);
        sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
    }
    else
    {
        sciDisplayText(sciREGx, (uint8_t*) "\r\nUDP App Alias init FAILED!", 28);
        sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
    }




    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));

    sciDisplayText(sciREGx, txtTitle, sizeof(txtTitle));
    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));

    sciDisplayText(sciREGx, txtTI, sizeof(txtTI));
    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
#ifdef __little_endian__        
    sciDisplayText(sciREGx, txtLittleEndian, sizeof(txtLittleEndian));
    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
#else        
    sciDisplayText(sciREGx, txtBigEndian, sizeof(txtBigEndian));
    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));


#endif

    temp_ip.addr = anaIpAddr;
    ipaddr_ntoa_r(&temp_ip, str_anaIp, sizeof(str_anaIp));
    txtIPAddrItoA = (uint8_t*) str_anaIp; // Terminale basilacak pointer'i ayarla

    if (sanalIpAddr != 0)
    {
        // Sanal IP'yi text'e cevir ve bizim str_sanalIp dizimize yaz
        temp_ip.addr = sanalIpAddr;
        ipaddr_ntoa_r(&temp_ip, str_sanalIp, sizeof(str_sanalIp));
        txtIPAddrItoA2 = (uint8_t*) str_sanalIp; // Ikinci pointer'i ayarla
    }

    sciDisplayText(sciREGx, txtIPAddrTxt, sizeof(txtIPAddrTxt));
    sciDisplayText(sciREGx, txtIPAddrItoA, 16);
    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));

    sciDisplayText(sciREGx, txtIPAddrTxt2, sizeof(txtIPAddrTxt2));
    sciDisplayText(sciREGx, txtIPAddrItoA2, 16);
    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));

    sciDisplayText(sciREGx, txtIPAddrTxt3, sizeof(txtIPAddrTxt3));
    sciDisplayText(sciREGx, txtIPAddrTxt3_IP, sizeof(txtIPAddrTxt3_IP));
    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));


    /* Loop forever.  All the work is done in interrupt handlers. */
    while (1)
    {

        /* UDP RX poll --- ISR'dan gelen paket varsa UART'a yazdir */
        {
            udp_rx_msg_t rx_0;
            if (udp_source_poll_rx(h_main, &rx_0))
            {
                char my_ip_str[16];
                ipaddr_ntoa_r(&g_main_netif->ip_addr, my_ip_str, sizeof(my_ip_str));

                char rx0_info[112];
                int n = snprintf(rx0_info, sizeof(rx0_info),
                                 "\r\nUDP RX: %u bytes, %s:%u -> %s:%u\r\n",
                                 (unsigned) rx_0.data_len,
                                 ipaddr_ntoa(&rx_0.src_ip), (unsigned) rx_0.src_port,
                                 my_ip_str, (unsigned) udp_source_get_local_port(h_main));
                sciDisplayText(sciREGx, (uint8_t*) rx0_info, (uint32_t) n);

                if (rx_0.data_len > 0)
                {
                    u16_t print_len = rx_0.data_len;
                    if (print_len > 64)
                        print_len = 64;
                    sciDisplayText(sciREGx, (uint8_t*) "Data: ", 6);
                    sciDisplayText(sciREGx, rx_0.data, (uint32_t) print_len);
                    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
                }
            }
        }

        {
            udp_rx_msg_t rx_1;
            if (udp_source_poll_rx(h_alias, &rx_1))
            {
                char my_ip_str[16];
                ipaddr_ntoa_r(&g_alias_netif->ip_addr, my_ip_str, sizeof(my_ip_str));

                char rx1_info[112];
                int n = snprintf(rx1_info, sizeof(rx1_info),
                                 "\r\nUDP RX: %u bytes, %s:%u -> %s:%u\r\n",
                                 (unsigned) rx_1.data_len,
                                 ipaddr_ntoa(&rx_1.src_ip), (unsigned) rx_1.src_port,
                                 my_ip_str, (unsigned) udp_source_get_local_port(h_alias));
                sciDisplayText(sciREGx, (uint8_t*) rx1_info, (uint32_t) n);

                if (rx_1.data_len > 0)
                {
                    u16_t print_len = rx_1.data_len;
                    if (print_len > 64)
                        print_len = 64;
                    sciDisplayText(sciREGx, (uint8_t*) "Data: ", 6);
                    sciDisplayText(sciREGx, rx_1.data, (uint32_t) print_len);
                    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
                }
            }
        }

        if (1 == send_main_flag)
        {
            send_main_flag = 0;
            err_t send_err = udp_data_send(h_main, g_main_netif, &dest, 5000,
                                           (const u8_t*) main_message,
                                           (u16_t) strlen(main_message));
            if (send_err != ERR_OK)
            {
                char err_msg[40];
                int el = snprintf(err_msg, sizeof(err_msg),
                                  "\r\nUDP send err: %d\r\n", (int) send_err);
                sciDisplayText(sciREGx, (uint8_t*) err_msg, (uint32_t) el);
            }
        }

        if (1 == send_alias_flag)
        {
            send_alias_flag = 0;
            err_t send_err = udp_data_send(h_alias, g_alias_netif, &dest, 4000,
                                           (const u8_t*) alias_message,
                                           (u16_t) strlen(alias_message));
            if (send_err != ERR_OK)
            {
                char err_msg[40];
                int el = snprintf(err_msg, sizeof(err_msg),
                                  "\r\nUDP send err: %d\r\n", (int) send_err);
                sciDisplayText(sciREGx, (uint8_t*) err_msg, (uint32_t) el);
            };
        }

        if(1 == terminal_input_flag){
            read_terminal_line();
            terminal_input_flag = 0;

        }
    }


}



void iommUnlock(void) {
	*(int *) 0xFFFFEA38  = 0x83E70B13;
    *(int *) 0xFFFFEA3C  = 0x95A4F1E0;
}

void iommLock(void) {
	*(int *) 0xFFFFEA38  = 0x00000000;
    *(int *) 0xFFFFEA3C  = 0x00000000;
}

void iommMuxEnableMdio(void) {
	*(int *) 0xFFFFEB2C  = 0x00000400; 
	*(int *) 0xFFFFEB30  = 0x00000400; 
}

void iommMuxEnableRmii(void) {
	*(int *) 0xFFFFEB38  = 0x02010204;
	*(int *) 0xFFFFEB3C  = 0x08020101;
	*(int *) 0xFFFFEB40  = 0x01010204;
	*(int *) 0xFFFFEB54  = 0x02040200;
	*(int *) 0xFFFFEB44  = 0x01080808;
	*(int *) 0xFFFFEB48  = 0x01010401;
}

void iommMuxEnableMii(void) {
	*(int *) 0xFFFFEB38  &= 0xFFFFFF00;
	*(int *) 0xFFFFEB38  |= (1 << 1);
	
	*(int *) 0xFFFFEB3C  &= 0x00FFFFFF;
	*(int *) 0xFFFFEB3C  |= (1 << 26);

	*(int *) 0xFFFFEB40  &= 0x0000FF00;
	*(int *) 0xFFFFEB40  |= ((1<<26) | (1<<18) | (1<<1));

	*(int *) 0xFFFFEB44  &= 0x00000000;
	*(int *) 0xFFFFEB44  |= ((1<<26)|(1<<18)|(1<<10)|(1<<2));

	*(int *) 0xFFFFEB48  &= 0xFFFF0000;
	*(int *) 0xFFFFEB48  |= ((1<<9)|(1<<2));

	*(int *) 0xFFFFEB54  &= 0xFF00FF00      ;
	*(int *) 0xFFFFEB54  |= ((1<<17)|(1<<1));

	*(int *) 0xFFFFEB5C  &= 0xFFFF00FF;
	*(int *) 0xFFFFEB5C  |= (1<<9);

	*(int *) 0xFFFFEB60  &= 0xFF00FFFF;
	*(int *) 0xFFFFEB60  |= (1<<18);

	*(int *) 0xFFFFEB84  &= 0x00FFFFFF;
	*(int *) 0xFFFFEB84  |= (0<<24);
}


/*
** Interrupt Handler for Core 0 Receive interrupt
** Degismedi: tek fiziksel instance (0) uzerinden hem ana hem sanal
** IP'ye ait paketler bu ISR araciligiyla girer; ayrimi ip_input()
** netif_list taramasiyla yapar.
*/
volatile int countEMACCore0RxIsr = 0;
#pragma INTERRUPT(EMACCore0RxIsr, IRQ)
void EMACCore0RxIsr(void)
{
		countEMACCore0RxIsr++;
		lwIPRxIntHandler(0);
}

/*
** Interrupt Handler for Core 0 Transmit interrupt
*/
volatile int countEMACCore0TxIsr = 0;
#pragma INTERRUPT(EMACCore0TxIsr, IRQ)
void EMACCore0TxIsr(void)
{
	countEMACCore0TxIsr++;
    lwIPTxIntHandler(0);
}

void IntMasterIRQEnable(void)
{
	_enable_IRQ();
	return;
}

void IntMasterIRQDisable(void)
{
	_disable_IRQ();
	return;
}

unsigned int IntMasterStatusGet(void)
{
    return (0xC0 & _get_CPSR());
}

void sciDisplayText(sciBASE_t *sci, uint8_t *text,uint32_t length)
{
    while(length--)
    {
        while ((sci->FLR & 0x4) == 4);
        sciSendByte(sci,*text++);
    };
}

void sciNotification(sciBASE_t *sci, uint32_t flags)
{
	return;
}


//Test functions to test the IP aliasing feature without connecting the ethernet port
#if 0

        /* Before printing the next set, wait for a character on the terminal */
        sciReceive(sciREGx, 1, &testChar);

        uint8_t rx_byte = 0; // rx_byte sıfırlanmalıdır ki önceki karakteri hatırlamasın

        /* Terminalden bir karakter bekle */
        sciReceive(sciREGx, 1, &rx_byte);


        /* ============================================================
         *  TEST KOMUTLARI
         *
         *  'p'  Tek ping testi (ayarlanabilir hedef IP/MAC)
         *  'd'  4 senaryolu tam diagnostik suiti
         *  't'  Elle parametreli ozel test
         * ============================================================ */

        if (rx_byte == 'p')
        {
            /* ---- Tek ping testi ----
             * Hedef IP/MAC'i degistirmek icin network_diagnostics.c
             * icindeki inject_fake_ping_packet() fonksiyonunun basindaki
             * USE_BOARD_IP / USE_BOARD_MAC / hedef_ip / hedef_mac
             * degerlerini degistirin. */
            inject_fake_ping_packet();
        }
        else if (rx_byte == 'd')
        {
            /* ---- Tam diagnostik suiti (4 test) ----
             * Test1: Dogru MAC + Ana IP
             * Test2: Dogru MAC + Yanlis IP
             * Test3: Yanlis MAC + Dogru IP
             * Test4: Dogru MAC + Alias IP  */
            run_ping_diagnostics();
        }

        else if (rx_byte == 't')
        {

            /* ---- Elle parametreli ozel test ----
             * Istediginiz IP ve MAC kombinasyonunu asagida degistirin. */
            ping_test_params_t test;
            int arp_sonuc, icmp_sonuc;

            /* Hedef MAC — kartin kendi MAC'i */
            test.dst_mac[0]=0x00; test.dst_mac[1]=0x08; test.dst_mac[2]=0xEE;
            test.dst_mac[3]=0x03; test.dst_mac[4]=0xA6; test.dst_mac[5]=0x6C;

            /* Kaynak MAC — sahte PC */
            test.src_mac[0]=0x11; test.src_mac[1]=0x22; test.src_mac[2]=0x33;
            test.src_mac[3]=0x44; test.src_mac[4]=0x55; test.src_mac[5]=0x66;

            /* Kaynak IP — sahte PC */
            test.src_ip[0]=192; test.src_ip[1]=168;
            test.src_ip[2]=2;   test.src_ip[3]=100;

            /* Hedef IP — TEST ETMEK ISTEDIGINIZ IP'YI BURAYA YAZIN */
            test.dst_ip[0]=192; test.dst_ip[1]=168;
            test.dst_ip[2]=2;   test.dst_ip[3]=50;    /*  alias IP */

            inject_realistic_ping_test(&test, netif_default, &arp_sonuc, &icmp_sonuc);

            sciDisplayText(sciREGx, (uint8_t*)"\r\nARP:  ", 7);
            if (arp_sonuc == 1)  sciDisplayText(sciREGx, (uint8_t*)"EVET\r\n", 6);
            else                 sciDisplayText(sciREGx, (uint8_t*)"HAYIR\r\n", 7);


            sciDisplayText(sciREGx, (uint8_t*)"ICMP: ", 6);
            if (icmp_sonuc == 1) sciDisplayText(sciREGx, (uint8_t*)"EVET\r\n", 6);
            else                 sciDisplayText(sciREGx, (uint8_t*)"HAYIR\r\n", 7);

        }

#endif
