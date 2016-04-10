/*
 *
 * Copyright (c) 2016 Raphine Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Author: Levelfour
 * 
 */

#include <global.h>
#include <posixtimer.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <net/socket.h>
#include <net/eth.h>
#include <net/arp.h>
#include <net/ip.h>
#include <net/udp.h>
#include <net/tcp.h>

int32_t NetSocket::Open() {
  _dev = reinterpret_cast<DevEthernet*>(netdev_ctrl->GetDevice());
  if(!_dev) {
    return -1;
  } else {
    return 0;
  }
}

/*
 * (TCP/IP) Socket
 */

uint32_t Socket::L2HeaderLength() { return sizeof(EthHeader); }
uint32_t Socket::L3HeaderLength() { return sizeof(IPv4Header); }
uint32_t Socket::L4HeaderLength() { return sizeof(TCPHeader); }

uint16_t Socket::L4Protocol() { return IPCtrl::kProtocolTCP; }

int32_t Socket::GetEthAddr(uint32_t ipaddr, uint8_t *macaddr) {
  while(arp_table->Find(ipaddr, macaddr) < 0) {
    ARPSocket socket;
    if(socket.Open() < 0) {
      return -1;
    } else {
      socket.TransmitPacket(ARPSocket::kOpARPRequest, ipaddr, nullptr);
    }
  }
  return 0;
}

int32_t Socket::L2Tx(uint8_t *buffer, uint8_t *saddr, uint8_t *daddr, uint16_t type) {
  return eth_ctrl->GenerateHeader(buffer, saddr, daddr, type);
}

bool Socket::L2Rx(uint8_t *buffer, uint8_t *saddr, uint8_t *daddr, uint16_t type) {
  return eth_ctrl->FilterPacket(buffer, saddr, daddr, type);
}

int32_t Socket::L3Tx(uint8_t *buffer, uint32_t length, uint8_t type, uint32_t saddr, uint32_t daddr) {
  return ip_ctrl->GenerateHeader(buffer, length, type, saddr, daddr);
}

bool Socket::L3Rx(uint8_t *buffer, uint8_t type, uint32_t saddr, uint32_t daddr) {
  return ip_ctrl->FilterPacket(buffer, type, saddr, daddr);
}

int32_t Socket::L4Tx(uint8_t *buffer, uint32_t length, uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport) {
  return tcp_ctrl->GenerateHeader(buffer, length, saddr, daddr, sport, dport, _type, _seq, _ack);
}

bool Socket::L4Rx(uint8_t *buffer, uint16_t sport, uint16_t dport) {
  return tcp_ctrl->FilterPacket(buffer, sport, dport, _type, _seq, _ack);
}

int32_t Socket::Transmit(const uint8_t *data, uint32_t length, bool is_raw_packet) {
  // alloc buffer
  NetDev::Packet *packet;
  if(!_dev->GetTxPacket(packet)) {
    return -1;
  }

  if(is_raw_packet) {
    packet->len = length;
    memcpy(packet->buf, data, length);
  } else {
    packet->len = L2HeaderLength() + L3HeaderLength() + L4HeaderLength() + length;

    // packet body
    uint32_t offset_body = L2HeaderLength() + L3HeaderLength() + L4HeaderLength();
    memcpy(packet->buf + offset_body, data, length);

    // TCP header
    uint32_t offset_l4 = L2HeaderLength() + L3HeaderLength();
    uint32_t saddr = _ipaddr;
    L4Tx(packet->buf + offset_l4, L4HeaderLength() + length, saddr, _daddr, _sport, _dport);

    // IP header
    uint32_t offset_l3 = L2HeaderLength();
    L3Tx(packet->buf + offset_l3, L4HeaderLength() + length, L4Protocol(), saddr, _daddr);

    // Ethernet header
    uint8_t eth_saddr[6];
    uint8_t eth_daddr[6] = {0x08, 0x00, 0x27, 0xc1, 0x5b, 0x93}; // TODO:
    _dev->GetEthAddr(eth_saddr);
//    GetEthAddr(_daddr, eth_daddr);
    L2Tx(packet->buf, eth_saddr, eth_daddr, EthCtrl::kProtocolIPv4);
  }

  // transmit
  _dev->TransmitPacket(packet);
  int32_t sent_length = packet->len;

  return (sent_length < 0 || is_raw_packet) ? sent_length : sent_length - (L2HeaderLength() + L3HeaderLength() + L4HeaderLength());
}

int32_t Socket::TransmitPacket(const uint8_t *packet, uint32_t length) {
  // total sent length
  int32_t sum = 0;
  // return value of receiving ack
  int32_t rval_ack = 0;
  // base count of round trip time
  uint64_t t0 = 0;

  while(true) {
    // length intended to be sent
    uint32_t send_length = length > kMSS ? kMSS : length;

    if(rval_ack != kErrorRetransmissionTimeout) t0 = timer->ReadMainCnt();

    // successfully sent length of packet
    int32_t rval = Transmit(packet, send_length, false);

    if(_type & kFlagACK) {
      // transmission mode is ACK
      uint8_t packet_ack[sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader)];
      uint8_t *tcp = packet_ack + sizeof(EthHeader) + sizeof(IPv4Header);

      if(rval >= 0 && _established) {

        uint64_t rto = timer->GetCntAfterPeriod(timer->ReadMainCnt(), GetRetransmissionTimeout());

        while(true) {
          // receive acknowledgement
          rval_ack = Receive(packet_ack, sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader), true, true, rto);
          if(rval_ack >= 0) {
            // acknowledgement packet received
            uint8_t type = tcp_ctrl->GetSessionType(tcp);
            uint32_t seq = tcp_ctrl->GetSequenceNumber(tcp);
            uint32_t ack = tcp_ctrl->GetAcknowledgeNumber(tcp);

            // sequence & acknowledge number validation
            if(type & kFlagACK && seq == _ack && ack == _seq + rval) {

              // calculate round trip time
              uint64_t t1 = timer->ReadMainCnt();
              _rtt_usec = t1 - t0;

              // seqeuence & acknowledge number is valid
              SetSequenceNumber(_seq + rval);

              sum += rval;
              break;
            }
          } else if(rval_ack == kErrorRetransmissionTimeout) {
            // retransmission timeout
            break;
          }
        }
      } else if(rval < 0) {
        // failed to transmit
        sum = rval;
        break;
      }
    }

    if(rval_ack == kErrorRetransmissionTimeout) continue;

    // for remaining segment transmission
    if(length > kMSS) {
      packet += rval;
      length -= rval;
      continue;
    } else {
      // transmission complete
      break;
    }
  }

  return sum;
}

int32_t Socket::TransmitRawPacket(const uint8_t *data, uint32_t length) {
  return Transmit(data, length, true);
}

int32_t Socket::Receive(uint8_t *data, uint32_t length, bool is_raw_packet, bool wait_timeout, uint64_t rto) {
  // receiving packet buffer
  DevEthernet::Packet *packet;
  // packet was on wire?
  bool packet_reached = false;
  // my MAC address
  uint8_t eth_daddr[6];
  _dev->GetEthAddr(eth_daddr);

  do {
    if(packet_reached) {
      // there is a packet already fetched but not released
      // discard it
      _dev->ReuseRxBuffer(packet);
      packet_reached = false;
    }

    if(_dev->ReceivePacket(packet)) {
      // packet on wire was fetched
      packet_reached = true;
    } else {
      // check retransmission timeout
      if(wait_timeout && rto <= timer->ReadMainCnt()) {
        return kErrorRetransmissionTimeout;
      } else {
        continue;
      }
    }

    // filter Ethernet address
    if(!L2Rx(packet->buf, nullptr, eth_daddr, EthCtrl::kProtocolIPv4)) continue;

    // filter IP address
    uint32_t offset_l3 = L2HeaderLength();
    if(!L3Rx(packet->buf + offset_l3 , L4Protocol(), _daddr, _ipaddr)) continue;

    // filter TCP port
    uint32_t offset_l4 = L2HeaderLength() + L3HeaderLength();
    if(!L4Rx(packet->buf + offset_l4, _sport, _dport)) continue;

    break;
  } while(true);

  // received "RAW" packet length
  int32_t received_length = packet->len;

  if(!is_raw_packet) {
    // copy data
    uint32_t offset = L2HeaderLength() + L3HeaderLength() + L4HeaderLength();
    memcpy(data, packet->buf + offset, length);
  } else {
    memcpy(data, packet->buf, packet->len < length ? packet->len : length);
  }

  // finalization
  _dev->ReuseRxBuffer(packet);

  return (received_length < 0 || is_raw_packet) ? received_length : received_length - (L2HeaderLength() + L3HeaderLength() + L4HeaderLength());
}

int32_t Socket::ReceivePacket(uint8_t *data, uint32_t length) {
  if(_type & kFlagACK) {
    // TCP acknowledgement
    uint32_t pkt_size = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader) + length;
    int32_t rval;
    uint8_t *packet = reinterpret_cast<uint8_t*>(virtmem_ctrl->Alloc(pkt_size));
    uint8_t *tcp = packet + sizeof(EthHeader) + sizeof(IPv4Header);

    while(true) {
      if((rval = Receive(packet, pkt_size, true, false, 0)) >= 0) {
        uint8_t type = tcp_ctrl->GetSessionType(tcp);
        uint32_t seq = tcp_ctrl->GetSequenceNumber(tcp);
        uint32_t ack = tcp_ctrl->GetAcknowledgeNumber(tcp);

        if(type & kFlagFIN) {
          SetSequenceNumber(ack);
          SetAcknowledgeNumber(seq + 1);
          CloseAck(type);
          rval = kErrorConnectionClosed;
          break;
        } else if(_ack == seq || (_seq == seq && _ack == ack)) {
          // acknowledge number = the expected next sequence number
          // (but the packet receiving right after 3-way handshake is not the case)
          rval -= sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader);
          SetSequenceNumber(ack);
          SetAcknowledgeNumber(seq + rval);

          // acknowledge
          if(_established) Transmit(nullptr, 0, false);
          memcpy(data, packet + pkt_size - length, length);
          break;
        } else {
          // something is wrong with the received sequence number
          // it is possible that sender retransmitted the lost packet,
          // so discard it and retry to receive next packet
          continue;
        }
      }
    }
    virtmem_ctrl->Free(reinterpret_cast<virt_addr>(packet));
    return rval;
  } else {
    return Receive(data, length, false, false, 0);
  }
}

int32_t Socket::ReceiveRawPacket(uint8_t *data, uint32_t length) {
  return Receive(data, length, true, false, 0);
}

int32_t Socket::Listen() {
  // connection already established
  if(_established) return -1;

  uint32_t kBufSize = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader);
  uint8_t buffer[kBufSize];
  uint8_t *tcp = buffer + sizeof(EthHeader) + sizeof(IPv4Header);
  uint32_t s, t;

  while(true) {
    // receive SYN packet
    SetSessionType(kFlagSYN);
    if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;
    t = tcp_ctrl->GetSequenceNumber(tcp);

    // transmit SYN+ACK packet
    SetSessionType(kFlagSYN | kFlagACK);
    s = rand();
    SetSequenceNumber(s);
    SetAcknowledgeNumber(++t);
    if(TransmitPacket(buffer, 0) < 0) continue;

    // receive ACK packet
    SetSessionType(kFlagACK);
    if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;

    // check sequence number
    if(tcp_ctrl->GetSequenceNumber(tcp) != t) continue;

    // check acknowledge number
    if(tcp_ctrl->GetAcknowledgeNumber(tcp) != s + 1) continue;

    break;
  }

  // connection established
  SetSequenceNumber(t);
  SetAcknowledgeNumber(s + 1);
  _established = true;

  return 0;
}

int32_t Socket::Connect() {
  // connection already established
  if(_established) return -1;

  uint32_t kBufSize = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader);
  uint8_t buffer[kBufSize];
  uint8_t *tcp = buffer + sizeof(EthHeader) + sizeof(IPv4Header);
  uint32_t s, t;

  while(true) {
    // transmit SYN packet
    t = rand();
    SetSessionType(kFlagSYN);
    SetSequenceNumber(t);
    SetAcknowledgeNumber(0);
    if(TransmitPacket(buffer, 0) < 0) continue;

    // receive SYN+ACK packet
    SetSessionType(kFlagSYN | kFlagACK);
    if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;

    // check acknowledge number
    s = tcp_ctrl->GetAcknowledgeNumber(tcp);
    if(s != t + 1) continue;

    // transmit ACK packet
    t = tcp_ctrl->GetSequenceNumber(tcp);
    SetSessionType(kFlagACK);
    SetSequenceNumber(s);
    SetAcknowledgeNumber(t + 1);
    if(TransmitPacket(buffer, 0) < 0) continue;

    break;
  }

  // connection established
  SetSequenceNumber(s);
  SetAcknowledgeNumber(t + 1);
  _established = true;

  return 0;
}

int32_t Socket::Close() {
  uint32_t kBufSize = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader);
  uint8_t buffer[kBufSize];
  uint8_t *tcp = buffer + sizeof(EthHeader) + sizeof(IPv4Header);
  uint32_t saved_seq = _seq;
  uint32_t saved_ack = _ack;
  uint32_t s = _seq;
  uint32_t t = _ack;

  while(true) {
    SetSequenceNumber(saved_seq);
    SetAcknowledgeNumber(saved_ack);

    // transmit FIN+ACK packet
    SetSessionType(kFlagFIN | kFlagACK);
    if(Transmit(buffer, 0, false) < 0) continue;

    // receive ACK packet
    SetSessionType(kFlagACK);
    if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;

    // check sequence number
    if(tcp_ctrl->GetSequenceNumber(tcp) != t) continue;

    // check acknowledge number
    if(tcp_ctrl->GetAcknowledgeNumber(tcp) != s + 1) continue;

    // receive FIN+ACK packet
    SetSessionType(kFlagFIN | kFlagACK);
    if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;

    // check sequence number
    if(tcp_ctrl->GetSequenceNumber(tcp) != t) continue;

    // check acknowledge number
    if(tcp_ctrl->GetAcknowledgeNumber(tcp) != s + 1) continue;

    // transmit ACK packet
    SetSessionType(kFlagACK);
    SetSequenceNumber(s + 1);
    SetAcknowledgeNumber(t + 1);
    if(Transmit(buffer, 0, false) < 0) continue;

    break;
  }

  _established = false;
  return 0;
}

int32_t Socket::CloseAck(uint8_t flag) {
  uint32_t kBufSize = sizeof(EthHeader) + sizeof(IPv4Header) + sizeof(TCPHeader);
  uint8_t buffer[kBufSize];
  uint8_t *tcp = buffer + sizeof(EthHeader) + sizeof(IPv4Header);

  if(flag == (kFlagFIN | kFlagACK)) {
    while(true) {
      // transmit ACK packet
      SetSessionType(kFlagACK);
      if(Transmit(buffer, 0, false) < 0) continue;

      // transmit FIN+ACK packet
      SetSessionType(kFlagFIN | kFlagACK);
      if(Transmit(buffer, 0, false) < 0) continue;

      // receive ACK packet
      SetSessionType(kFlagACK);
      if(ReceiveRawPacket(buffer, kBufSize) < 0) continue;

      // check sequence number
      if(tcp_ctrl->GetSequenceNumber(tcp) != _ack) continue;

      // check acknowledge number
      if(tcp_ctrl->GetAcknowledgeNumber(tcp) != _seq + 1) continue;

      break;
    }
  }
 
  _established = false;
  return 0;
}
/*
 * UDPSocket
 */

uint32_t UDPSocket::L4HeaderLength() { return sizeof(UDPHeader); }

uint16_t UDPSocket::L4Protocol() { return IPCtrl::kProtocolUDP; }

int32_t UDPSocket::L4Tx(uint8_t *buffer, uint32_t length, uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport) {
  return udp_ctrl->GenerateHeader(buffer, length, sport, dport);
}

bool UDPSocket::L4Rx(uint8_t *buffer, uint16_t sport, uint16_t dport) {
  return udp_ctrl->FilterPacket(buffer, sport, dport);
}

int32_t UDPSocket::ReceivePacket(uint8_t *data, uint32_t length) {
  return Receive(data, length, false, false, 0);
}

int32_t UDPSocket::TransmitPacket(const uint8_t *data, uint32_t length) {
  return Transmit(data, length, false);
}

/*
 * ARPSocket
 */

void ARPSocket::SetIPAddr(uint32_t ipaddr) {
  _ipaddr = ipaddr;
}

int32_t ARPSocket::TransmitPacket(uint16_t type, uint32_t tpa, uint8_t *tha) {
  uint32_t ip_daddr = tpa;
  uint8_t eth_saddr[6];
  uint8_t eth_daddr[6];
  _dev->GetEthAddr(eth_saddr);

  switch(type) {
    case kOpARPRequest:
      memset(eth_daddr, 0xff, 6); // broadcast
      break;
    case kOpARPReply:
      memcpy(eth_daddr, tha, 6);
      break;
    default:
      // unknown ARP operation
      return -1;
  }

  // alloc buffer
  DevEthernet::Packet *packet;
  if(!_dev->GetTxPacket(packet)) {
    return -1;
  }
  uint32_t len = sizeof(EthHeader) + sizeof(ARPPacket);
  packet->len = len;

  // ARP header
  uint32_t offsetARP = sizeof(EthHeader);
  arp_ctrl->GeneratePacket(packet->buf + offsetARP, type, eth_saddr, _ipaddr, eth_daddr, ip_daddr);

  // Ethernet header
  eth_ctrl->GenerateHeader(packet->buf, eth_saddr, eth_daddr, EthCtrl::kProtocolARP);

  // transmit
  _dev->TransmitPacket(packet);

  return type;
}

int32_t ARPSocket::ReceivePacket(uint16_t type, uint32_t *spa, uint8_t *sha) {
  // alloc buffer
  DevEthernet::Packet *packet;
  int16_t op = 0;

  uint8_t eth_daddr[6];
  _dev->GetEthAddr(eth_daddr);

  do {
    // receive
    if(!_dev->ReceivePacket(packet)) continue;

    // filter Ethernet address
    if(!eth_ctrl->FilterPacket(packet->buf, nullptr, eth_daddr, EthCtrl::kProtocolARP)) continue;

    // filter IP address
    if(!arp_ctrl->FilterPacket(packet->buf + sizeof(EthHeader), type, nullptr, 0, eth_daddr, _ipaddr)) continue;

    break;
  } while(true);

  uint8_t *p = packet->buf + sizeof(EthHeader) + kOperationOffset;
  op = ntohs(*reinterpret_cast<uint16_t*>(p));

  // handle received ARP request/reply
  switch(op) {
    case kOpARPReply:
      arp_ctrl->RegisterAddress(packet->buf + sizeof(EthHeader));
    case kOpARPRequest:
      if(spa) *spa = arp_ctrl->GetSourceIPAddress(packet->buf + sizeof(EthHeader));
      if(sha) arp_ctrl->GetSourceMACAddress(sha, packet->buf + sizeof(EthHeader));
      break;
    default:
      _dev->ReuseRxBuffer(packet);
      return -1;
  }

  // finalization
  _dev->ReuseRxBuffer(packet);

  return op;
}