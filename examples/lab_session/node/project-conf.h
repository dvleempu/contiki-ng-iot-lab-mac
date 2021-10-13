/*
 * Copyright (c) 2015, SICS Swedish ICT.
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
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* CSMA TUNING */
#define CSMA_CONF_SEND_SOFT_ACK                     1
#define CSMA_CONF_RTS_CTS                           1
#define CSMA_CONF_MIN_BE                            3
#define CSMA_CONF_MAX_BE                            5

/* TDMA TUNING */
#define MAC_CONF_WITH_TSCH                          0
#define TSCH_CONF_WITH_EACK                         0
#define APP_SLOTFRAME_SIZE                          4

/* *** DO NOT TOUCH! *** */
#define CSMA_CONF_MAX_PACKET_PER_NEIGHBOR           4
#define CSMA_CONF_MAX_FRAME_RETRIES                 CSMA_CONF_SEND_SOFT_ACK
#define SICSLOWPAN_CONF_FRAG                        0
#define UIP_CONF_BUFFER_SIZE                        160
#define TSCH_SCHEDULE_CONF_WITH_6TISCH_MINIMAL      0
#define TSCH_SCHEDULE_CONF_MAX_LINKS                APP_SLOTFRAME_SIZE
#define TSCH_CONF_EB_PERIOD                         (TSCH_SCHEDULE_CONF_MAX_LINKS * CLOCK_SECOND/10)
#define TSCH_CONF_DEFAULT_HOPPING_SEQUENCE          TSCH_HOPPING_SEQUENCE_1_1
#define QUEUEBUF_CONF_NUM                           4
#define TSCH_CONF_MAC_MAX_FRAME_RETRIES             1

#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_ERR
#define LOG_CONF_LEVEL_FRAMER                      LOG_LEVEL_NONE

#endif /* PROJECT_CONF_H_ */
