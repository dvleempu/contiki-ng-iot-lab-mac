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
 *         Interferer node
 * \author
*         Dries Van Leemput <dries.vanleemput@ugent.be>
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
#define SEND_INTERVAL (CLOCK_SECOND)/29
#define PACKET_SIZE   120
uint8_t data[PACKET_SIZE] = {9};

/*---------------------------------------------------------------------------*/
PROCESS(interferer_process, "Interferer node");
AUTOSTART_PROCESSES(&interferer_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(interferer_process, ev, data)
{
  static struct etimer periodic_timer;

  PROCESS_BEGIN();

  NETSTACK_RADIO.init();
  NETSTACK_RADIO.off();

  etimer_set(&periodic_timer, INIT_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      // LOG_INFO("Sending\n");
      NETSTACK_RADIO.send(data, PACKET_SIZE);
      NETSTACK_RADIO.off();
    /* Add some jitter */
    // etimer_set(&periodic_timer, SEND_INTERVAL);
      etimer_set(&periodic_timer, SEND_INTERVAL
      - SEND_INTERVAL/30 + (random_rand() % (2 * SEND_INTERVAL/30)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
