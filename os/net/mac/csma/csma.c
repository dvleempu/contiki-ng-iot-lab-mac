/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
*         The 802.15.4 standard CSMA protocol (nonbeacon-enabled)
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Simon Duquennoy <simon.duquennoy@inria.fr>
 */

#include "net/mac/csma/csma.h"
#include "net/mac/csma/csma-output.h"
#include "net/mac/mac-sequence.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "net/mac/framer/framer-802154.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSMA"
#define LOG_LEVEL LOG_LEVEL_MAC

#if CSMA_RTS_CTS
static struct packetbuf_attr ctsbuf_attrs[PACKETBUF_NUM_ATTRS];
static int seqnoo;
static uint8_t rts_busy = 0;
static clock_time_t cts_set;
static clock_time_t cts_now;
#endif /* CSMA_RTS_CTS */

static void
init_sec(void)
{
#if LLSEC802154_USES_AUX_HEADER
  if(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL) ==
     PACKETBUF_ATTR_SECURITY_LEVEL_DEFAULT) {
    packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL,
                       CSMA_LLSEC_SECURITY_LEVEL);
  }
#endif
}
/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{

  init_sec();

  csma_output_packet(sent, ptr);
}
/*---------------------------------------------------------------------------*/
#if CSMA_RTS_CTS
void
ctsbuf_set_attr(uint8_t type, const packetbuf_attr_t val)
{
  ctsbuf_attrs[type].val = val;
  return;
}
/*---------------------------------------------------------------------------*/
/* Return the value of a specified attribute */
packetbuf_attr_t
ctsbuf_attr(uint8_t type)
{
  return ctsbuf_attrs[type].val;
}
/*---------------------------------------------------------------------------*/
int
create_cts(uint8_t *buf, uint16_t buf_len, 
                const linkaddr_t *dest_addr, uint8_t seqno)
{
  frame802154_t params;
  int hdr_len;
  int cts_len;

  if(buf == NULL) {
    return -1;
  }

  memset(ctsbuf_attrs, 0, sizeof(ctsbuf_attrs));

  ctsbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_CTSFRAME);
  ctsbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, seqno);

  if(dest_addr != NULL) {
    ctsbuf_set_attr(PACKETBUF_ATTR_MAC_NO_DEST_ADDR, 0);
    linkaddr_copy((linkaddr_t *)&params.dest_addr, dest_addr);
  }
  ctsbuf_set_attr(PACKETBUF_ATTR_MAC_NO_SRC_ADDR, 1);

  framer_802154_setup_params(ctsbuf_attr, 0, &params);
  hdr_len = frame802154_hdrlen(&params);

  memset(buf, 0, buf_len);

  cts_len = hdr_len;

  frame802154_create(&params, buf);

  return cts_len;

}
/*---------------------------------------------------------------------------*/
static void cts() {

  static uint8_t cts_buf[PACKETBUF_SIZE];
  static int cts_len;
  const linkaddr_t *addr = packetbuf_addr(PACKETBUF_ADDR_SENDER);

  cts_len = create_cts(cts_buf, sizeof(cts_buf), addr, seqnoo);
  seqnoo++;

  if(cts_len > 0) {
    LOG_DBG("Transmitting CTS to ");
    LOG_DBG_LLADDR(addr);
    LOG_DBG_("\n");
    NETSTACK_RADIO.prepare((const void *)cts_buf, cts_len);
    NETSTACK_RADIO.transmit(cts_len);
  }
  cts_set = clock_time();
}
#endif /* CSMA_RTS_CTS */
/*---------------------------------------------------------------------------*/
static void
input_packet(void)
{
#if CSMA_SEND_SOFT_ACK
  uint8_t ackdata[CSMA_ACK_LEN];
#endif

#if CSMA_RTS_CTS
  cts_now = clock_time();
  if(rts_busy && (cts_now-cts_set < RTIMER_SECOND / 200)) {
    rts_busy = 0;
    LOG_DBG("CTS timer expired\n");
  }
#endif /* CSMA_RTS_CTS */
  if(packetbuf_datalen() == CSMA_ACK_LEN) {
    /* Ignore ack packets */
    LOG_DBG("ignored ack\n");
  } else if(csma_security_parse_frame() < 0) {
    LOG_WARN("failed to parse %u\n", packetbuf_datalen());
  } else if(!linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &linkaddr_node_addr) &&
            !packetbuf_holds_broadcast()) {
    LOG_WARN("not for us\n");
#if CSMA_RTS_CTS
    /* Wait if CTS is received for other frame*/
    if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_CTSFRAME) {
      LOG_DBG("Received CTS, wait for packet + ACK\n");

      /* Wait for max CSMA_ACK_WAIT_TIME */
      RTIMER_BUSYWAIT_UNTIL(NETSTACK_RADIO.pending_packet(), CSMA_PACKET_ACK_WAIT_TIME);

      if(NETSTACK_RADIO.receiving_packet() ||
          NETSTACK_RADIO.pending_packet() ||
          NETSTACK_RADIO.channel_clear() == 0) {
        int len;
        uint8_t ackbuf[CSMA_ACK_LEN];

        /* Wait an additional CSMA_AFTER_ACK_DETECTED_WAIT_TIME to complete reception */
        RTIMER_BUSYWAIT_UNTIL(NETSTACK_RADIO.pending_packet(), CSMA_AFTER_ACK_DETECTED_WAIT_TIME);

        if(NETSTACK_RADIO.pending_packet()) {
          len = NETSTACK_RADIO.read(ackbuf, CSMA_ACK_LEN);
          if(len == CSMA_ACK_LEN) {
            /* Ack received */
            LOG_DBG("ACK wait done\n");
          }
        }        
      }
    }
#endif /* CSMA_RTS_CTS */
  } else if(linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_SENDER), &linkaddr_node_addr)) {
    LOG_WARN("frame from ourselves\n");
#if CSMA_RTS_CTS
  } else if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_RTSFRAME) {
    if(linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &linkaddr_node_addr)) {
      if(!rts_busy) {
        LOG_DBG("Received RTS from ");
        LOG_DBG_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
        LOG_DBG_("\n");
        rts_busy = 1;
        cts();
      } else {
        LOG_DBG("Ignored RTS, already handling request\n");
      }
    } else {
      LOG_DBG("Ignored RTS, not for us\n");
    }
#endif /* CSMA_RTS_CTS */
  } else {
    int duplicate = 0;

    /* Check for duplicate packet. */
    duplicate = mac_sequence_is_duplicate();
    if(duplicate) {
      /* Drop the packet. */
      LOG_WARN("drop duplicate link layer packet from ");
      LOG_WARN_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
      LOG_WARN_(", seqno %u\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO));
    } else {
      mac_sequence_register_seqno();
    }

#if CSMA_SEND_SOFT_ACK
    if(packetbuf_attr(PACKETBUF_ATTR_MAC_ACK)) {
      ackdata[0] = FRAME802154_ACKFRAME;
      ackdata[1] = 0;
      ackdata[2] = ((uint8_t *)packetbuf_hdrptr())[2];
      NETSTACK_RADIO.send(ackdata, CSMA_ACK_LEN);
    }
#endif /* CSMA_SEND_SOFT_ACK */
#if CSMA_RTS_CTS
    rts_busy = 0;
#endif /* CSMA_RTS_CTS */
    if(!duplicate) {
      LOG_INFO("received packet from ");
      LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
      LOG_INFO_(", seqno %u, len %u\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO), packetbuf_datalen());
      NETSTACK_NETWORK.input();
    }
  }
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return NETSTACK_RADIO.on();
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return NETSTACK_RADIO.off();
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  radio_value_t radio_max_payload_len;

  /* Check that the radio can correctly report its max supported payload */
  if(NETSTACK_RADIO.get_value(RADIO_CONST_MAX_PAYLOAD_LEN, &radio_max_payload_len) != RADIO_RESULT_OK) {
    LOG_ERR("! radio does not support getting RADIO_CONST_MAX_PAYLOAD_LEN. Abort init.\n");
    return;
  }

#if CSMA_SEND_SOFT_ACK
  radio_value_t radio_rx_mode;

  /* Disable radio driver's autoack */
  if(NETSTACK_RADIO.get_value(RADIO_PARAM_RX_MODE, &radio_rx_mode) != RADIO_RESULT_OK) {
    LOG_WARN("radio does not support getting RADIO_PARAM_RX_MODE\n");
  } else {
    /* Unset autoack */
    radio_rx_mode &= ~RADIO_RX_MODE_AUTOACK;
    if(NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, radio_rx_mode) != RADIO_RESULT_OK) {
      LOG_WARN("radio does not support setting RADIO_PARAM_RX_MODE\n");
    }
  }
#endif


#if LLSEC802154_USES_AUX_HEADER
#ifdef CSMA_LLSEC_DEFAULT_KEY0
  uint8_t key[16] = CSMA_LLSEC_DEFAULT_KEY0;
  csma_security_set_key(0, key);
#endif
#endif /* LLSEC802154_USES_AUX_HEADER */
  csma_output_init();
  on();
}
/*---------------------------------------------------------------------------*/
static int
max_payload(void)
{
  int framer_hdrlen;
  radio_value_t max_radio_payload_len;
  radio_result_t res;

  init_sec();

  framer_hdrlen = NETSTACK_FRAMER.length();

  res = NETSTACK_RADIO.get_value(RADIO_CONST_MAX_PAYLOAD_LEN,
                                 &max_radio_payload_len);

  if(res == RADIO_RESULT_NOT_SUPPORTED) {
    LOG_ERR("Failed to retrieve max radio driver payload length\n");
    return 0;
  }

  if(framer_hdrlen < 0) {
    /* Framing failed, we assume the maximum header length */
    framer_hdrlen = CSMA_MAC_MAX_HEADER;
  }

  return MIN(max_radio_payload_len, PACKETBUF_SIZE)
    - framer_hdrlen
    - LLSEC802154_PACKETBUF_MIC_LEN();
}
/*---------------------------------------------------------------------------*/
const struct mac_driver csma_driver = {
  "CSMA",
  init,
  send_packet,
  input_packet,
  on,
  off,
  max_payload,
};
/*---------------------------------------------------------------------------*/
