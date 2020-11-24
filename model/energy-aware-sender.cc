
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

#include "ns3/assert.h"
#include "ns3/callback.h"
#include "ns3/energy-aware-sender.h"
#include "ns3/pointer.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/lora-net-device.h"
#include "src/lorawan/model/lora-radio-energy-model.h"

namespace ns3 {
  namespace lorawan {

    NS_LOG_COMPONENT_DEFINE ("EnergyAwareSender");

    NS_OBJECT_ENSURE_REGISTERED (EnergyAwareSender);

    TypeId
    EnergyAwareSender::GetTypeId (void)
    {
      static TypeId tid = TypeId ("ns3::EnergyAwareSender")
        .SetParent<Application> ()
        .AddConstructor<EnergyAwareSender> ()
        .SetGroupName ("lorawan")
        .AddAttribute ("EnergyThreshold", "The energy threshold over which sending the packet",
                       DoubleValue (0),
                       MakeDoubleAccessor (&EnergyAwareSender::GetEnergyThreshold,
                                           &EnergyAwareSender::SetEnergyThreshold),
                       MakeDoubleChecker<double> ())
      .AddAttribute ("MinInterval", "The minimum interval between packet sends of this app",
                     TimeValue (Seconds (0)),
                     MakeTimeAccessor (&EnergyAwareSender::GetMinInterval,
                                       &EnergyAwareSender::SetMinInterval),
                     MakeTimeChecker ());
      // .AddAttribute ("PacketSizeRandomVariable", "The random variable that determines the shape of the packet size, in bytes",
      //                StringValue ("ns3::UniformRandomVariable[Min=0,Max=10]"),
      //                MakePointerAccessor (&EnergyAwareSender::m_pktSizeRV),
      //                MakePointerChecker <RandomVariableStream>());
      return tid;
    }

    EnergyAwareSender::EnergyAwareSender ()
      : m_energyThreshold (0),
        m_sendTime (Seconds(0)),
        m_firstSending(true),
        m_tryingToSend (false),
        m_basePktSize (10),
        m_pktSizeRV (0)
    {
      NS_LOG_FUNCTION_NOARGS ();
    }

    EnergyAwareSender::~EnergyAwareSender ()
    {
      NS_LOG_FUNCTION_NOARGS ();
    }

    void
    EnergyAwareSender::SetEnergyThreshold (double energyThreshold)
    {
      NS_LOG_FUNCTION (this << energyThreshold);
      m_energyThreshold = energyThreshold;
    }

    double 
    EnergyAwareSender::GetEnergyThreshold (void) const
    {
      NS_LOG_FUNCTION (this);
      return m_energyThreshold;
    }

    void
    EnergyAwareSender::SetMinInterval (Time interval)
    {
      NS_LOG_FUNCTION (this << interval);
      m_interval = interval;
    }

    Time 
    EnergyAwareSender::GetMinInterval (void) const
    {
      NS_LOG_FUNCTION (this);
      return m_interval;
    }

    void
    EnergyAwareSender::SetPacketSizeRandomVariable (Ptr <RandomVariableStream> rv)
    {
      m_pktSizeRV = rv;
    }


    void
    EnergyAwareSender::SetPacketSize (uint8_t size)
    {
      m_basePktSize = size;
    }


    void
    EnergyAwareSender::SendPacket (void)
    {
      NS_LOG_FUNCTION (this);

      // Create and send a new packet
      Ptr<Packet> packet;
      if (m_pktSizeRV)
        {
          int randomsize = m_pktSizeRV->GetInteger ();
          packet = Create<Packet> (m_basePktSize + randomsize);
        }
      else
        {
          packet = Create<Packet> (m_basePktSize);
        }

      m_sendTime = Simulator::Now();

      NS_LOG_DEBUG ("Sent a packet of size " << packet->GetSize () << " m_sendTime (s) "
                                             << m_sendTime.GetSeconds ());
      m_tryingToSend = true;

      m_mac->Send (packet);

      // Schedule the next SendPacket event
      // m_sendEvent = Simulator::Schedule (m_interval, &EnergyAwareSender::SendPacket,
      //                                    this);
    }

    void
    EnergyAwareSender::StartApplication (void)
    {
      NS_LOG_FUNCTION (this);

      // Make sure we have a MAC layer
      if (m_mac == 0)
        {
          // Assumes there's only one device
          Ptr<LoraNetDevice> loraNetDevice = m_node->GetDevice (0)->GetObject<LoraNetDevice> ();

          m_mac = loraNetDevice->GetMac ();
          NS_ASSERT (m_mac != 0);

        }

      // Assume there's a loraRadioEnergyModel
      Ptr<LoraRadioEnergyModel> radioEnergy = m_node->GetObject<EnergySourceContainer> ()
        ->Get (0)
        ->FindDeviceEnergyModels ("ns3::LoraRadioEnergyModel")
        .Get (0)
        -> GetObject<LoraRadioEnergyModel> ();
      NS_ASSERT (radioEnergy != 0);

      NS_LOG_DEBUG ("Connect the callback");
      radioEnergy-> SetEnergyChangedCallback
        (MakeCallback(&EnergyAwareSender::EnergyAwareSendPacketCallback, this));

      // Schedule the next SendPacket event
      // Simulator::Cancel (m_sendEvent);
      // NS_LOG_DEBUG ("Starting up application with a first event with a " <<
      //               m_initialDelay.GetSeconds () << " seconds delay");
      // m_sendEvent = Simulator::Schedule (m_initialDelay,
      //                                    &EnergyAwareSender::SendPacket, this);
      // NS_LOG_DEBUG ("Event Id: " << m_sendEvent.GetUid ());
    }

    void
    EnergyAwareSender::StopApplication (void)
    {
      NS_LOG_FUNCTION_NOARGS ();
      // Simulator::Cancel (m_sendEvent);
    }

    // Callback
    void
    EnergyAwareSender::EnergyAwareSendPacketCallback (double newEnergy)
    {
      // Send packet only if a time has passed
      if (m_firstSending == 1 || (Simulator::Now () - m_sendTime > m_interval))
        {
          if (m_tryingToSend)
            {
              NS_LOG_DEBUG ("We are already trying to send a packet");
            }
          else if (newEnergy >= m_energyThreshold)
            {
              NS_LOG_DEBUG ("Enough Energy to send a packet");
              SendPacket ();
              m_firstSending = 0;
            }
          else
            {
              NS_LOG_DEBUG ("Not enough energy");
            }
        }
      else
        {
          NS_LOG_DEBUG ("Enough Energy to send a packet, but too frequent");
        }
    }

    void
    EnergyAwareSender::PhyStartedSendingCallback (Ptr<Packet const> packet, uint32_t id)
    {
      NS_LOG_FUNCTION (packet << id);
      m_tryingToSend = false;
    }
  }
}
