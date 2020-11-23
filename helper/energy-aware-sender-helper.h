
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

#ifndef ENERGY_AWARE_SENDER_HELPER_H
#define ENERGY_AWARE_SENDER_HELPER_H

#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/energy-aware-sender.h"
#include <stdint.h>
#include <string>

namespace ns3 {
namespace lorawan {

/**
 * This class can be used to install EnergyAwareSender applications on a wide
 * range of nodes.
 */
class EnergyAwareSenderHelper
{
public:
  EnergyAwareSenderHelper ();

  ~EnergyAwareSenderHelper ();

  void SetAttribute (std::string name, const AttributeValue &value);

  ApplicationContainer Install (NodeContainer c) const;

  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Set the energy threshold.
   *
   * \param energyThreshold
   */
  void SetEnergyThreshold (double energyThreshold);

  void SetMinInterval (Time interval);

  void SetPacketSizeRandomVariable (Ptr <RandomVariableStream> rv);

  void SetPacketSize (uint8_t size);


private:
  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory;

  Time m_interval;
  double m_energyThreshold;

  Ptr<RandomVariableStream> m_pktSizeRV; // whether or not a random component is added to t he packet size

  uint8_t m_pktSize; // the packet size.

};

} // namespace ns3

}
#endif /* ENERGY_AWARE_SENDER_HELPER_H */
