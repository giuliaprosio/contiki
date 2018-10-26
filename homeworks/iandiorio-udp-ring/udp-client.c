#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_PORT	8765
#define UDP_SERVER_PORT	5678
#define TIMER_INTERVAL 4 * CLOCK_SECOND

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL		  (60 * CLOCK_SECOND)

#define TRUE 1
#define FALSE 0
#define MX_NEW_CONNECTION 6
#define MX_NEW_SUCCESSOR 5
#define MX_MX 4
#define MX_FIRST_CLIENT 3

static struct simple_udp_connection udp_conn;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
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

static uip_ipaddr_t successor_address;
static int is_first_time = TRUE;
static int is_first_node = FALSE;

void initialize(const uip_ipaddr_t *server_address);
void save_new_successor(address_msg_t message);
void broadcast_message(normal_msg_t message);
void restart_ring();
void to_first_position();

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
  LOG_INFO("Received response %u from ", message_type);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");

  switch(message_type) {
    case MX_FIRST_CLIENT:
      to_first_position();
      break;
    case MX_NEW_SUCCESSOR:{
      address_msg_t address;
      address = *(address_msg_t *) data;
      save_new_successor(address);
      break;
    }
    case MX_MX:{
      normal_msg_t message;
      message = *(normal_msg_t *) data;
      broadcast_message(message);
      break;
    }
    default:
      break;  
  }
  
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_PORT, NULL,
                      UDP_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      if(is_first_time) {
        initialize(&dest_ipaddr);
        is_first_time = FALSE;
      } else if (is_first_node) {
        restart_ring();
      }
    } else {
      LOG_INFO("Not reachable yet\n");
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

void initialize(const uip_ipaddr_t *server_addr) {
  empty_msg_t mx;
  mx.type = MX_NEW_CONNECTION;
  simple_udp_sendto(&udp_conn, &mx, sizeof(empty_msg_t), server_addr);
}

void save_new_successor(address_msg_t message) {
  successor_address = message.address;
  LOG_INFO("SUCCESSOR:   ");
  LOG_INFO_6ADDR(&successor_address);
  LOG_INFO("\n");
}

void broadcast_message(normal_msg_t message) {
  uip_ipaddr_t succ_addr = successor_address;
  LOG_INFO("Sending ring message to ");
  LOG_INFO_6ADDR(&succ_addr);
  LOG_INFO("\n");
  simple_udp_sendto(&udp_conn, &message, sizeof(normal_msg_t), &succ_addr);
}

void restart_ring() {
  normal_msg_t message;
  message.type = MX_MX;
  message.value = random_rand();
  LOG_INFO("Starting new ring with value %d.\n", message.value);
  broadcast_message(message);
}

void to_first_position() {
  is_first_node = TRUE;
}