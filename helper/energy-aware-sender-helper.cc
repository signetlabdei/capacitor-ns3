
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

#include "ns3/energy-aware-sender-helper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/energy-aware-sender.h"
#include "ns3/lora-net-device.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("EnergyAwareSenderHelper");

EnergyAwareSenderHelper::EnergyAwareSenderHelper ()
{
  m_factory.SetTypeId ("ns3::EnergyAwareSender");

  m_energyThreshold = 0.5;
  m_pktSize = 10;
  m_pktSizeRV = 0;
}

EnergyAwareSenderHelper::~EnergyAwareSenderHelper ()
{
}

void
EnergyAwareSenderHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
EnergyAwareSenderHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
EnergyAwareSenderHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
EnergyAwareSenderHelper::InstallPriv (Ptr<Node> node) const
{
  NS_LOG_FUNCTION (this << node-> GetId());

  Ptr<EnergyAwareSender> app = m_factory.Create<EnergyAwareSender> ();

  app->SetEnergyThreshold (m_energyThreshold);
  app->SetMinInterval (m_interval);
  NS_LOG_DEBUG ("Created an application with energy threshold= " <<
                m_energyThreshold << " J and min interval " <<
                m_interval.GetSeconds() << " s");

  app->SetPacketSize (m_pktSize);
  if (m_pktSizeRV)
    {
      app->SetPacketSizeRandomVariable (m_pktSizeRV);
    }

  app->SetNode (node);
  node->AddApplication (app);

  node->GetDevice(0)->GetObject<LoraNetDevice> ()->GetPhy()->TraceConnectWithoutContext("StartSending",
                                                                                        MakeCallback(&EnergyAwareSender::PhyStartedSendingCallback, app));
  node->GetDevice (0)->GetObject<LoraNetDevice> ()->GetPhy ()->TraceConnectWithoutContext (
      "SendingNotPossible",
      MakeCallback (&EnergyAwareSender::PhySendingNotPossibleCallback, app));

  return app;
}

void
EnergyAwareSenderHelper::SetEnergyThreshold (double energyThreshold)
{
  m_energyThreshold = energyThreshold;
}

void
EnergyAwareSenderHelper::SetMinInterval (Time interval)
{
  m_interval = interval;
}

void
EnergyAwareSenderHelper::SetPacketSizeRandomVariable (Ptr <RandomVariableStream> rv)
{
  m_pktSizeRV = rv;
}

void
EnergyAwareSenderHelper::SetPacketSize (uint8_t size)
{
  m_pktSize = size;
}

}
} // namespace ns3
