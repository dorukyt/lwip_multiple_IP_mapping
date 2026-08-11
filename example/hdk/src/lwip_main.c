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
uint8_t     txtTitle[]          = {"HERCULES MICROCONTROLLERS"};
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

void smallDelay(void) {
	  static volatile unsigned int delayval;
	  delayval = 10000;
	  while(delayval--);
}

void EMAC_LwIP_Main (uint8_t * macAddress)
{
    //uint8_t 		testChar;
    struct in_addr 	devIPAddress;
    unsigned int	anaIpAddr;

    /* --- Statik IP tanimlari ---
     * ip_ana : cihazin birincil (default route'un ciktigi) adresi
     * ip_sanal: ayni fiziksel EMAC + ayni MAC uzerinde ikinci (alias) IP
     */
    uint8_t ip_ana[4]      = { 10, 0, 0, 10  };
    uint8_t netmask_ana[4] = { 255, 255, 255, 0 };
    uint8_t gateway_ana[4] = { 10, 0, 0, 1 };

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

	struct netif *g_main_netif  = lwIPNetifPtrGet(0);
	struct netif *g_alias_netif = lwIPAliasNetifPtrGet(0);

	sciDisplayText(sciREGx, (uint8_t*)"..DONE", sizeof("..DONE"));
	sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));

	devIPAddress.s_addr = anaIpAddr;
	txtIPAddrItoA = (uint8_t *)inet_ntoa(devIPAddress);

	LocatorConfig(macAddress, "HDK enet_lwip (dual-ip)");

	//sciDisplayText(sciREGx, (uint8_t*)"Starting Web Server", sizeof("Starting Web Server"));
	//httpd_init();
	sciDisplayText(sciREGx, (uint8_t*)"..DONE", sizeof("..DONE"));
	sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));

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

    /* --- Read the current IP address from the structures and write them in the terminal --- */
    sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
    terminal_list_netif();

    /* Loop forever.  All the work is done in interrupt handlers. */
    while (1)
    {


        //rx_polling
        //polls in the loop to get the data that is present at the UDP socket buffer
        //writes down the package specifications and sender receiver information
        {
            u8_t j;
            udp_rx_msg_t rx_msg;
            udp_netif_socket_info_t socket;
            struct netif *n;

            for(n = netif_list; n != NULL; n = n->next){        //search every netif
                for(j = 0; j < udp_source_count_sockets(n); j++)//get all the sockets on netif
                {                                               //(searches every socket avaible on every netif)
                    socket = udp_source_get_socket(n, j);

                    if (udp_source_poll_rx(socket.socket_id, &rx_msg))
                    {
                        char my_ip_str[16];
                        ipaddr_ntoa_r(&socket.netif->ip_addr, my_ip_str,
                                      sizeof(my_ip_str));

                        char rx_info[112];
                        int msg_len = snprintf(
                                rx_info, sizeof(rx_info),
                                "\r\n#EVT UDPRX %s:%u -> %s:%u %u bayt\r\n",
                                ipaddr_ntoa(&rx_msg.src_ip),
                                (unsigned) rx_msg.src_port, my_ip_str,
                                (unsigned) socket.local_port,
                                (unsigned) rx_msg.data_len);
                        sciDisplayText(sciREGx, (uint8_t*) rx_info,
                                       (uint32_t) msg_len);

                        if (rx_msg.data_len > 0)
                        {
                            u16_t print_len = rx_msg.data_len;
                            if (print_len > 64)
                                print_len = 64;
                            sciDisplayText(sciREGx, (uint8_t*) "#EVT Data: ", 11);
                            sciDisplayText(sciREGx, rx_msg.data,
                                           (uint32_t) print_len);
                            sciDisplayText(sciREGx, txtCRLF, sizeof(txtCRLF));
                        }

                    }

                }
            }
        }

        /* 'q' seri porttan gelince menuyu ac ('sciIsRxReady' guard'i ile bloklamaz) */
        if (sciIsRxReady(sciREGx))
        {
            uint8_t key = (uint8_t) sciReceiveByte(sciREGx);
            if (key == 'q')
            {
                terminal_input_flag = 1;
            }
            else
            {
                cmd_channel_feed(key);   // 'q' disindaki baytlar -> yapisal komut kanali
            }
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
