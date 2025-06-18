#include <stdint.h>
#include <stdio.h>

#include "sensor-data.h"

#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "net/netstack.h"
#include "net/routing/routing.h"
#include "net/routing/rpl-lite/rpl-dag-root.h"
#include "sys/log.h"

#define LOG_MODULE "Server"
#define LOG_LEVEL  LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT   8765
#define UDP_SERVER_PORT   5678

#undef LOG_CONF_LEVEL_RPL
#define LOG_CONF_LEVEL_RPL LOG_LEVEL_INFO
#define LOG_INTERVAL       (60 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

/*---------------------------------------------------------------------------*/
PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port, const uint8_t *data,
                            uint16_t datalen) {
  if (datalen != sizeof(sensor_data_t)) {
    return;
  }
  sensor_data_t *payload = (sensor_data_t *)data;

  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_(" > humidity: " QSTR "%% | temperature: " QSTR
            " ºC | visible light: %d lm | total light: %d lm\n",
            Q2STR(payload->rh), Q2STR(payload->temp), payload->light_visible,
            payload->light_total);

#if WITH_SERVER_REPLY
  LOG_INFO("Sending response.\n");
  simple_udp_sendto(&udp_conn, data, datalen, sender_addr);
#endif /* WITH_SERVER_REPLY */
}

PROCESS_THREAD(udp_server_process, ev, data) {
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  NETSTACK_ROUTING.root_start();

  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT,
                      udp_rx_callback);

  etimer_set(&periodic_timer, LOG_INTERVAL);

  while (true) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    rpl_dag_root_print_links("Server");

    etimer_set(&periodic_timer, LOG_INTERVAL);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
