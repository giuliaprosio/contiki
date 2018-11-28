#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "stdlib.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define CLIENT_PORT 8765
#define SERVER_PORT 5678
#define IEEE802154_CONF_DEFAULT_CHANNEL 15

#define ACK_VALUE 1

#define SWITCH_INTERVAL (20 * CLOCK_SECOND)

typedef struct
{
  int seconds;
} seconds_on_t;

static struct simple_udp_connection udp_conn;

/*---------------------------------------------------------------------------*/
PROCESS(hard_state_client_process, "Soft state client");
AUTOSTART_PROCESSES(&hard_state_client_process);
/*---------------------------------------------------------------------------*/

static int has_sent_request = 0;

static void udp_rx_callback(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr, uint16_t sender_port, const uip_ipaddr_t *receiver_addr, uint16_t receiver_port, const unsigned char *data, uint16_t datalen)
{

  uint8_t ack = *(uint8_t *)data;
  LOG_INFO("I have received ack %u\n", ack);

  if (ack == ACK_VALUE)
  {
    has_sent_request = 0;
  }
}

PROCESS_THREAD(hard_state_client_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, CLIENT_PORT, NULL, SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, SWITCH_INTERVAL);

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if (NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr))
    {

      if (!has_sent_request)
      {

        int seconds = rand() % 60 + 1;

        if (seconds < 0)
        {
          seconds = seconds * -1;
        }
        seconds_on_t message;
        message.seconds = seconds;

        LOG_INFO("Sending to stay on for %d seconds to ", message.seconds);
        LOG_INFO_6ADDR(&dest_ipaddr);
        LOG_INFO_("\n");
        simple_udp_sendto(&udp_conn, &message, sizeof(int), &dest_ipaddr);
        has_sent_request = 1;
      }
    }
    else
    {
      LOG_INFO("Not reachable yet!\n");
    }
    etimer_set(&periodic_timer, SWITCH_INTERVAL);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
