
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Padova
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Martina Capuzzo <capuzzom@dei.unipd.it>
 */

#ifndef ENERGY_AWARE_SENDER_H
#define ENERGY_AWARE_SENDER_H

#include "ns3/application.h"
#include "ns3/nstime.h"
#include "ns3/lorawan-mac.h"
#include "ns3/attribute.h"

namespace ns3 {
namespace lorawan {

class EnergyAwareSender : public Application
{
public:
  EnergyAwareSender ();
  ~EnergyAwareSender ();

  static TypeId GetTypeId (void);

  /**
   * Set the energy threshold over which sending the packet
   * \param threshold The energy that must be in the source to allow sending the packet
   */
  void SetEnergyThreshold (double threshold);

  /**
   * Get the energy threshold
   * \returns the energy threshold
   */
  double GetEnergyThreshold (void) const;

  void SetMinInterval(Time interval);
  Time GetMinInterval (void) const;
  
   
  /**
   * Set packet size
   */
  void SetPacketSize (uint8_t size);

  /**
   * Set if using randomness in the packet size
   */
  void SetPacketSizeRandomVariable (Ptr <RandomVariableStream> rv);

  /**
   * Send a packet using the LoraNetDevice's Send method
   */
  void SendPacket (void);

  /**
   * Start the application by scheduling the first SendPacket event
   */
  void StartApplication (void);

  /**
   * Stop the application
   */
  void StopApplication (void);

private:
  void EnergyAwareSendPacketCallback (double);

private:
  
  double m_energyThreshold;
  Time m_interval;
  Time m_sendTime;
  bool m_firstSending;


  /**
   * The MAC layer of this node
   */
  Ptr<LorawanMac> m_mac;

  /**
   * The packet size.
   */
  uint8_t m_basePktSize;


  /**
   * The random variable that adds bytes to the packet size
   */
  Ptr<RandomVariableStream> m_pktSizeRV;


};

} //namespace ns3

}
#endif /* SENDER_APPLICATION */