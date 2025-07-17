#include <inttypes.h>
#include <stdlib.h>

#include "sensor-data.h"

#include "contiki.h"
#include "light-sensor.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-ds6.h"
#include "net/netstack.h"
#include "net/routing/routing.h"
#include "random.h"
#include "sht11-sensor.h"
#include "sys/log.h"

#define LOG_MODULE "Client"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define SEND_INTERVAL (10 * CLOCK_SECOND)

static uint32_t rx_count;
static struct simple_udp_connection udp_conn;

static int16_t read_temp(void);
static int16_t read_rh(void);
static int16_t read_light_visible(void);
static int16_t read_light_total(void);

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void udp_rx_callback(struct simple_udp_connection *c,
                            const uip_ipaddr_t *sender_addr,
                            uint16_t sender_port,
                            const uip_ipaddr_t *receiver_addr,
                            uint16_t receiver_port, const uint8_t *data,
                            uint16_t datalen) {
  LOG_INFO("Received response from server.\n");
  rx_count++;
}

PROCESS_THREAD(udp_client_process, ev, data) {
  static struct etimer periodic_timer;
  static uip_ipaddr_t dest_ipaddr;
  static uint32_t tx_count;
  static uint32_t missed_tx_count;

  PROCESS_BEGIN();

  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT,
                      udp_rx_callback);

  SENSORS_ACTIVATE(sht11_sensor);
  SENSORS_ACTIVATE(light_sensor);

  // Ciclo de sensoriamento
  sensor_data_t payload = {0};
  while (true)
    ;

  SENSORS_DEACTIVATE(sht11_sensor);
  SENSORS_DEACTIVATE(light_sensor);

  PROCESS_END();
}

/*---------------------------------------------------------------------------*/

static int16_t read_temp(void) {
  return -3960 + sht11_sensor.value(SHT11_SENSOR_TEMP);
}

static int16_t read_rh(void) {
  int16_t so_rh = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);

  /*
   * The datasheet coefficients should be multiplied by 10_000 to retain
   * first order information.
   *
   * rh = c1 + c2 * so_rh + c3 * (so_rh)^2
   *
   * Given the small c3 coefficient, it needs further scaling of 1_000 so
   * the quadratic term may be expanded to:
   *
   * so_rh * so_rh * (c3 * 1_000) / 1_000
   *
   * fixing this order of operations to keep numeric precision.
   */

  // clang-format off

  // Quadratic term
  int32_t res = (int32_t)so_rh * (int32_t)so_rh * 7L / 250L;
  res = -40000L + 405L * (int32_t)so_rh - res;

  // clang-format on

  // Rescale from 10_000 to 100
  return (int16_t)(res / 100);
}

static int16_t read_light_visible(void) {
  return light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
}

static int16_t read_light_total(void) {
  return light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR);
}
