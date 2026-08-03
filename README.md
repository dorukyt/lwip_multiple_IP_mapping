# Runtime Multi-IP UDP Networking — TMS570LC43 / lwIP

Runtime-configurable multiple-IP networking for the TI Hercules **TMS570LC43** (LAUNCHXL2),
built on **lwIP 1.4.1** (bare-metal, `NO_SYS`). Network interfaces and UDP sockets can be
created, configured, and destroyed **at runtime** from a serial terminal — no recompilation.
Includes ICMP ping and terminal-driven UDP send.

---

## Table of Contents
1. [What This Is](#what-this-is)
2. [Features](#features)
3. [Requirements](#requirements)
4. [Architecture](#architecture)
5. [Module Map](#module-map)
6. [Key Concepts](#key-concepts)
7. [Build & Flash](#build--flash)
8. [Serial Terminal Usage](#serial-terminal-usage)
9. [Configuration](#configuration)
10. [Public APIs (for extending)](#public-apis)
11. [Testing & Verifying](#testing--verifying)
12. [Known Limitations](#known-limitations)
13. [Roadmap](#roadmap)
14. [Glossary](#glossary)

---

## What This Is
The board runs a single physical Ethernet MAC (EMAC) but exposes **multiple IP addresses**
via **IP aliasing** — several lwIP `netif`s sharing one EMAC/MAC. Originally these interfaces
and their UDP sockets were fixed at compile time. This project makes the whole thing
**dynamic and terminal-driven**: you add/remove interfaces, open/close UDP sockets on any
interface, edit addresses, send/receive UDP, and ping — all live, over the serial console.

## Features
- **Dynamic netifs** — add / remove / reconfigure (IP, netmask, gateway) alias interfaces at runtime.
- **Dynamic UDP sockets** — open / close sockets on any netif; each is a full bidirectional endpoint.
- **Automatic RX** — the main loop polls the whole socket registry, so any socket you open is
  received automatically.
- **Terminal-driven UDP send** — pick a socket, enter `IP/port`, type a message.
- **ICMP ping** — echo request/reply over the RAW API with round-trip-time measurement.
- **Safe lifecycle** — removing a netif cascades to tear down its sockets; the physical netif is
  protected from removal; stale socket handles are detected (generational ids).

## Requirements
| Item | Value |
|---|---|
| Board | TI Hercules **TMS570LC43** (LAUNCHXL2-TMS570LC43) |
| TCP/IP stack | lwIP **1.4.1**, `NO_SYS = 1` (bare-metal), big-endian |
| Drivers | HALCoGen-generated (`HL_` prefix) |
| Toolchain / IDE | TI ARM compiler, Code Composer Studio (CCS 12.x) |
| Debug probe | XDS110 (on-board) |
| Serial | SCI/UART (`sciREG1`), a terminal at the board's configured baud |

## Architecture
    ┌───────────────────────────────────────────────┐
    │        terminal_interface.c  (user menu)       │
    │  configure / add / remove netif, open / close  │
    │  socket, send UDP, ping                         │
    └───────────────┬───────────────────────────────┘
                    │ calls
    ┌───────────────▼───────────┐   ┌──────────────────┐
    │  net_manager.c            │   │  ping.c          │
    │  net_if_add / net_if_remove│   │  ICMP echo (RAW) │
    │  (cascade + guards)       │   └────────┬─────────┘
    └───────┬───────────┬───────┘            │
            │           │                    │
            ┌────────────▼──┐ ┌────▼───────────────┐ │
            │ lwiplib.c │ │ UDP_source.c │ │
            │ alias netif │ │ socket registry │ │
            │ add/remove │ │ (id + generation) │ │
            └───────┬───────┘ └─────────┬──────────┘ │
            │ │ │
            ┌──▼─────────────────────▼───────────────▼──┐
            │ lwIP 1.4.1 core │
            │ netif_list · udp_pcb · raw_pcb · etharp │
            └────────────────────┬───────────────────────┘
            │
            ┌────────▼────────┐
            │ EMAC + PHY │ (one physical MAC,
            │ (HALCoGen) │ shared by all netifs)
            └──────────────────┘

- **Main loop** (`lwip_main.c`): initializes hardware + lwIP, then loops forever polling all
  sockets for RX and handling terminal input. EMAC RX/TX run in ISRs.
- **Routing**: incoming packets are steered to the correct netif by `netif_alias_route_filter`
  (walks `netif_list` matching the destination IP).

## Module Map
| File | Responsibility |
|---|---|
| `example/hdk/src/lwip_main.c` | App entry (`EMAC_LwIP_Main`): init, main loop (RX poll + terminal), EMAC ISRs |
| `lwip-1.4.1/ports/hdk/lwiplib.c` | lwIP core init + **alias netif pool** (`lwIPAliasAdd`/`Remove`, `lwIPNetifPtrGet`) |
| `src/UDP_source.c` / `include/UDP_source.h` | **UDP socket registry** — id/generation, open/close/send/poll, per-netif enumeration |
| `src/net_manager.c` / `include/net_manager.h` | Thin orchestration — `net_if_add` / `net_if_remove` (cascade) |
| `src/terminal_interface.c` / `include/terminal_interface.h` | Serial menu + all user commands |
| `src/ping.c` / `include/ping.h` | ICMP echo (ping) over the RAW API |
| `src/netif_alias_filter.c` | Incoming-packet routing across aliased netifs |
| `example/hdk/inc/lwipopts.h` | lwIP configuration overrides (see below) |

## Key Concepts
- **IP aliasing** — multiple `netif`s on one EMAC/MAC. The **primary (physical) netif** owns the
  hardware (PHY reset/autoneg happen only for it) and is **never removable**. **Alias netifs**
  copy the physical netif's output path and share it; they are the dynamic layer.
- **Generational socket ids** — a socket id packs `(generation << 8) | slot_index`. When a slot
  is reused, its generation increments, so an old (stale) id is detected and rejected. All id
  handling goes through one validation gate, `resolve()`.
- **Registry, not linked lists** — the "sockets of a netif" relationship isn't stored; it's
  derived on demand by scanning the fixed socket array (`s_listeners[]`). Same for netifs.
- **ISR-safe teardown** — the EMAC RX callback runs in interrupt context, so removals do
  `udp_remove` first (stops callbacks) then clear state, inside `SYS_ARCH_PROTECT` critical
  sections. Every `PROTECT` pairs with exactly one `UNPROTECT`.
- **Cascade** — `net_if_remove` removes a netif's sockets **before** the netif, so no socket is
  ever left bound to a removed interface.

## Build & Flash
1. Open the project in **Code Composer Studio** (CCS 12.x) and import this repository.
2. Ensure all source folders are in the build (notably `src/`, `example/hdk/src/`,
   `lwip-1.4.1/...`). New files must be part of the CCS project to be compiled.
3. **Build** (Project → Build). It should compile clean.
4. Connect the board via USB (XDS110) and **Flash/Debug**.
5. Open a serial terminal on the board's SCI/UART port. Press the on-board button
   (GPIOB[5]) to bring up the menu.

> If the debugger fails with an XDS110 DLL / `SC_ERR_LIB_LOAD_LOCAL` error, it's a probe/driver
> issue (not the code): close CCS, re-plug the XDS110, or reboot the PC.

## Serial Terminal Usage
Press the on-board button to open the menu, then choose a command by number:

| Command | What it does |
|---|---|
| **Configure Netif** | Select a netif, then edit its **IP / netmask / gateway** (IP edit re-binds sockets). |
| **Reset ARP tables** | Clears the ARP cache on all netifs. |
| **Ping** | Select a source netif, enter a target IP, sends ICMP echo (4 tries) and shows RTT/TTL. |
| **Add network interface** | Enter IP, netmask, gateway → creates a new alias netif. |
| **Delete network interface** | Select a netif → removes it (and cascades its sockets). |
| **Open Socket** | Select a netif, enter a port → opens a UDP socket on it. |
| **Delete Socket** | Select a netif, then a socket → closes it. |
| **Send data** | Select a sender socket, enter `IP/port` (e.g. `192.168.0.1/5000`), type a message → sends UDP. |

Received UDP is printed automatically by the main loop (source IP:port → local IP:port + data).

> Menu selection currently reads a single character, so choices are limited to `1`–`9`.

## Configuration
lwIP options are overridden in **`example/hdk/inc/lwipopts.h`** — never edit lwIP's `opt.h`
directly. `opt.h` uses `#ifndef X / #define X default / #endif`, and `lwipopts.h` is included
first, so anything you define there wins.

| Macro | Where | Meaning |
|---|---|---|
| `UDP_MAX_LISTENERS` | `UDP_source.h` | Max sockets in the registry (default 16). Fits in the id's low byte, so ≤ 256. |
| `MEMP_NUM_UDP_PCB` | `lwipopts.h` | Size of lwIP's udp_pcb pool. **Shared** across the whole stack (the locator holds one), so keep it **> `UDP_MAX_LISTENERS`** (set to 20). |
| `MAX_ALIAS_NETIF` | `lwiplib.c` | Max alias netifs. |
| `LWIP_RAW`, `LWIP_ICMP` | (default 1) | Required for ping. |

## Public APIs
Use these to build on top of the project.

**Interface lifecycle** (`net_manager.h`)
```c
err_t net_if_add(ip_addr_t *ip, ip_addr_t *mask, ip_addr_t *gw);
err_t net_if_remove(struct netif *netif);   // cascades sockets, guards the physical netif



**UDP sockets** (UDP_source.h)
err_t udp_source_add_listener(struct netif *netif, u16_t port, udp_sock_id_t *id_out);
void  udp_source_remove_listener(udp_sock_id_t id);
u8_t  udp_source_poll_rx(udp_sock_id_t id, udp_rx_msg_t *msg);
err_t udp_source_data_send(udp_sock_id_t id, struct netif *tx_netif,
                           ip_addr_t *dst, u16_t dst_port, const u8_t *data, u16_t len);
u8_t  udp_source_count_sockets(struct netif *netif);
udp_netif_socket_info_t udp_source_get_socket(struct netif *netif, u8_t nth);
void  udp_source_remove_all_on_netif(struct netif *netif);


Alias netifs (lwiplib.h)
unsigned int  lwIPAliasAdd(unsigned int primaryInst, unsigned int ip, unsigned int mask, unsigned int gw);
void          lwIPAliasRemove(struct netif *netif);
struct netif *lwIPNetifPtrGet(unsigned int inst);        // the physical netif
struct netif *lwIPAliasNetifPtrGet(unsigned int idx);


Ping (ping.h)
err_t ping_send(struct netif *netif, ip_addr_t *target);
u8_t  ping_wait_reply(u32_t timeout_ms, ping_result_t *out);


Testing & Verifying
Wireshark — capture on the interface facing the board. Filter e.g. udp.port == 5000 or
ip.addr == <board_ip>. Inspect the IP (src/dst) and UDP (src/dst port) layers and the Data
payload. Expect an ARP request/reply on the first send.
netcat (to actually receive a sent message): nc -u -l 5000 on a PC in the board's subnet.
ping — from the board (menu) to a PC, or from a PC to the board's IP.
Known Limitations
Shared udp_pcb pool — sockets share MEMP_NUM_UDP_PCB with the rest of the stack (e.g. the
locator). Keep the pool above UDP_MAX_LISTENERS.
Single-character menu — choices limited to 1–9.
Physical netif is non-removable — it owns the hardware and the aliases depend on it.
Big-endian target — htonl/ntohl are no-ops here but kept explicit for portability.
Roadmap
ARP-based network scan (host discovery) — etharp_query + etharp_find_addr.
Open-port scan (TCP/UDP).
Service/version detection (banner grabbing) over lwIP TCP.
Glossary
netif — lwIP network interface (one IP address + config).
alias netif — an extra netif sharing the physical EMAC/MAC.
udp_pcb — lwIP's UDP "connection" control block.
socket id — this project's handle for a registry socket: generation + slot index.
cascade — removing a netif also removing all its sockets.
ISR — Interrupt Service Routine (EMAC RX/TX run here).
