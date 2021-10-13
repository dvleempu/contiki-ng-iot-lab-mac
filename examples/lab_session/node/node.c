/*
 * Copyright (c) 2017, RISE SICS.
 * All rights reserved.
 *
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

/**
 * \file
 *         NullNet unicast example
 * \author
*         Simon Duquennoy <simon.duquennoy@ri.se>
 *
 */

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "random.h"
#include "sys/node-id.h"
#include "net/mac/tsch/tsch.h"

#include <string.h>
#include <stdio.h> /* For printf() */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Configuration */
#define INIT_INTERVAL (CLOCK_SECOND)*2
#define SEND_INTERVAL (CLOCK_SECOND)/60
#define PACKET_SIZE   20
static linkaddr_t dest_addr =         {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};

#if MAC_CONF_WITH_TSCH
#include "net/mac/tsch/tsch.h"
static linkaddr_t coordinator_addr =  {{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }};
/* Put all cells on the same slotframe */
#define APP_SLOTFRAME_HANDLE 1
/* Put all unicast cells on the same timeslot (for demonstration purposes only) */
#define APP_UNICAST_TIMESLOT 1
#endif /* MAC_CONF_WITH_TSCH */

/*---------------------------------------------------------------------------*/
PROCESS(node_process, "Node example");
AUTOSTART_PROCESSES(&node_process);

#if MAC_CONF_WITH_TSCH
static void
initialize_tsch_schedule(void)
{
  int i;
  struct tsch_slotframe *sf_common = tsch_schedule_add_slotframe(APP_SLOTFRAME_HANDLE, APP_SLOTFRAME_SIZE);
  uint16_t slot_offset;
  uint16_t channel_offset;

  slot_offset = 0;
  channel_offset = 0;
  tsch_schedule_add_link(sf_common,
      LINK_OPTION_RX | LINK_OPTION_TX | LINK_OPTION_SHARED,
      LINK_TYPE_ADVERTISING_ONLY, &tsch_broadcast_address,
      slot_offset, channel_offset, 1);

  for (i = 0; i < TSCH_SCHEDULE_MAX_LINKS - 1; ++i) {
    slot_offset = i+1;
    channel_offset = i;

    if(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr)) {
      tsch_schedule_add_link(sf_common,
        LINK_OPTION_RX,
        LINK_TYPE_NORMAL, &coordinator_addr,
        slot_offset, channel_offset, 1);
    }
    if((int)linkaddr_node_addr.u8[0] == i+2) {
      tsch_schedule_add_link(sf_common,
        LINK_OPTION_TX,
        LINK_TYPE_NORMAL, &coordinator_addr,
        slot_offset, channel_offset, 1);
    }
  }
}
#endif /* MAC_CONF_WITH_TSCH */

/*---------------------------------------------------------------------------*/
void input_callback(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest)
{
  unsigned count[PACKET_SIZE];
  memcpy(&count, data, sizeof(count));
  LOG_INFO("Received %u from ", count[PACKET_SIZE-1]);
  LOG_INFO_LLADDR(src);
  LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count[PACKET_SIZE] = {0};

  PROCESS_BEGIN();

#if MAC_CONF_WITH_TSCH
  tsch_set_coordinator(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr));
  initialize_tsch_schedule();
#endif /* MAC_CONF_WITH_TSCH */

  /* Initialize NullNet */
  nullnet_buf = (uint8_t *)&count;
  nullnet_len = sizeof(count);
  nullnet_set_input_callback(input_callback);

  if(!linkaddr_cmp(&dest_addr, &linkaddr_node_addr)) {
    etimer_set(&periodic_timer, INIT_INTERVAL);
    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
#if MAC_CONF_WITH_TSCH
      if(tsch_is_associated) {
#endif /* MAC_CONF_WITH_TSCH */
        LOG_INFO("Sending %u to ", count[PACKET_SIZE-1]);
        LOG_INFO_LLADDR(&dest_addr);
        LOG_INFO_("\n");

        NETSTACK_NETWORK.output(&dest_addr);
        count[PACKET_SIZE-1]++;
#if MAC_CONF_WITH_TSCH
      }
#endif /* MAC_CONF_WITH_TSCH */
      /* Add some jitter */
      etimer_set(&periodic_timer, SEND_INTERVAL);
      // etimer_set(&periodic_timer, SEND_INTERVAL
      //   - SEND_INTERVAL/60 + (random_rand() % (2 * SEND_INTERVAL/60)));
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
