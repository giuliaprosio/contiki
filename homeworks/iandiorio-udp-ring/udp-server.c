/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_PORT	8765
#define UDP_SERVER_PORT	5678
#define TIMER_INTERVAL 4 * CLOCK_SECOND

static struct simple_udp_connection udp_conn;

#define TRUE 1
#define FALSE 0
#define MX_NEW_CONNECTION 6
#define MX_NEW_SUCCESSOR 5
#define MX_MX 4
#define MX_FIRST_CLIENT 3

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/

typedef struct {
  uint8_t type;
} empty_msg_t;

typedef struct {
  uint8_t type;
  uip_ipaddr_t address;
} address_msg_t;

typedef struct {
  uint8_t type;
  int value;
} normal_msg_t;

static uip_ipaddr_t first_node;
static uip_ipaddr_t last_node;
static int is_first = TRUE;

void handle_new_connection(const uip_ipaddr_t *sender_addr);
void handle_end_of_ring(normal_msg_t message, const uip_ipaddr_t *sender_addr);

void save_first_connection(const uip_ipaddr_t *sender_addr);
void save_last_node(const uip_ipaddr_t *sender_addr);
void send_first_client_notification();
void send_new_address(const uip_ipaddr_t *sender_addr);
void send_root_address();

static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const unsigned char *data,
         uint16_t datalen)
{
  uint8_t message_type = *(unsigned *)data;
  switch(message_type) {
    case MX_NEW_CONNECTION:
      handle_new_connection(sender_addr);
      break;
    case MX_MX:{
      normal_msg_t message = *(normal_msg_t *) data;
      handle_end_of_ring(message, sender_addr);
      break;
    }
    default:
      LOG_INFO("Unrecognized Message \n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_PORT, NULL,
                      UDP_PORT, udp_rx_callback);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

void handle_new_connection(const uip_ipaddr_t *sender_addr) {
  if(!is_first) {
    uip_ipaddr_t sender_address = *sender_addr;
    send_new_address(sender_addr);
    save_last_node(&sender_address);
    send_root_address();
  } else {
    uip_ipaddr_t sender_address_one = *sender_addr;
    uip_ipaddr_t sender_address_two = *sender_addr;
    save_first_connection(&sender_address_one);
    send_first_client_notification();
    save_last_node(&sender_address_two);
    is_first = FALSE;
  }
}

void send_new_address(const uip_ipaddr_t *sender_addr) {
  address_msg_t mx;
  mx.type = MX_NEW_SUCCESSOR;
  mx.address = *sender_addr;
  uip_ipaddr_t dest_addr = last_node;
  simple_udp_sendto(&udp_conn, &mx, sizeof(address_msg_t), &dest_addr);
}

void save_last_node(const uip_ipaddr_t *sender_addr) {
    last_node = *sender_addr;
}

void send_root_address() {
  address_msg_t mx;
  mx.type = MX_NEW_SUCCESSOR;
  NETSTACK_ROUTING.get_root_ipaddr(&mx.address);
  uip_ipaddr_t dest_addr = last_node;
  simple_udp_sendto(&udp_conn, &mx, sizeof(address_msg_t), &dest_addr);
}

void save_first_connection(const uip_ipaddr_t *sender_addr) {
  first_node = *sender_addr;
}

void send_first_client_notification() {
  empty_msg_t mx;
  mx.type = MX_FIRST_CLIENT;
  uip_ipaddr_t dest_addr = first_node;
  simple_udp_sendto(&udp_conn, &mx, sizeof(empty_msg_t), &dest_addr);
}

void handle_end_of_ring(normal_msg_t mx, const uip_ipaddr_t *sender_addr) {
  LOG_INFO("LAST NODE, RING COMPLETED \n\n");

}

