
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Network Security Lab, University of Washington, Seattle.
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
 * Authors: Martina Capuzzo <capuzzom@dei.unipd.it>
 */

#include "capacitor-energy-source.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include "ns3/packet.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include <math.h>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CapacitorEnergySource");

NS_OBJECT_ENSURE_REGISTERED (CapacitorEnergySource);

TypeId
CapacitorEnergySource::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::CapacitorEnergySource")
          .SetParent<EnergySource> ()
          .SetGroupName ("Energy")
          .AddConstructor<CapacitorEnergySource> ()
          .AddAttribute ("Capacity", "Capacity of the capacitor [F]",
                         DoubleValue (0.01), // 10 mF
                         MakeDoubleAccessor (&CapacitorEnergySource::m_capacity),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("CapacitorEnergySourceInitialVoltageV",
                         "Initial voltage of the capacitor.",
                         DoubleValue (0), // in Volt
                         MakeDoubleAccessor (&CapacitorEnergySource::SetInitialVoltage,
                                             &CapacitorEnergySource::GetInitialVoltage),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("CapacitorEnergySourceInitialEnergyJ",
                         "Initial energy of the capacitor energy source.",
                         DoubleValue (0), // in Volt
                         MakeDoubleAccessor (&CapacitorEnergySource::SetInitialEnergy),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("CapacitorEnergySupplyVoltageV",
                         "Max Supply voltage for capacitor energy source.",
                         DoubleValue (3.3), // in Volts
                         MakeDoubleAccessor (&CapacitorEnergySource::m_supplyVoltageV),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("CapacitorLowVoltageThreshold",
                         "Low voltage threshold for capacitor energy source.",
                         DoubleValue (0.0), // as a fraction of the initial voltage
                         MakeDoubleAccessor (&CapacitorEnergySource::m_lowVoltageTh),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("CapacitorHighVoltageThreshold",
                         "High voltage threshold for capacitor energy source.",
                         DoubleValue (1), // as a fraction of the initial voltage
                         MakeDoubleAccessor (&CapacitorEnergySource::m_highVoltageTh),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("PeriodicVoltageUpdateInterval",
                         "Time between two consecutive periodic voltage updates.",
                         TimeValue (Seconds (1.0)),
                         MakeTimeAccessor (&CapacitorEnergySource::SetUpdateInterval,
                                           &CapacitorEnergySource::GetUpdateInterval),
                         MakeTimeChecker ())
          .AddTraceSource ("RemainingEnergy", "Remaining energy at CapacitorEnergySource.",
                           MakeTraceSourceAccessor (&CapacitorEnergySource::m_remainingEnergyJ),
                           "ns3::TracedValueCallback::Double")
          .AddTraceSource ("RemainingVoltage", "Remaining voltage at CapacitorEnergySource.",
                           MakeTraceSourceAccessor (&CapacitorEnergySource::m_actualVoltageV),
                           "ns3::TracedValueCallback::Double");
  return tid;
}

CapacitorEnergySource::CapacitorEnergySource ()
{
  NS_LOG_FUNCTION (this);
  m_lastUpdateTime = Seconds (0.0);
  m_depleted = false;
}

CapacitorEnergySource::~CapacitorEnergySource ()
{
  NS_LOG_FUNCTION (this);
}

void
CapacitorEnergySource::SetInitialVoltage (double initialVoltageV)
{
  NS_LOG_FUNCTION (this << initialVoltageV);
  NS_ASSERT (initialVoltageV >= 0);

  if (m_initialVoltageV> m_supplyVoltageV)
    {
      m_initialVoltageV= m_supplyVoltageV;
    }
  else
    {
      m_initialVoltageV = initialVoltageV;
    }

  m_actualVoltageV = m_initialVoltageV;
  double initialEnergy = m_capacity*pow(m_initialVoltageV, 2)/2;
  m_remainingEnergyJ = initialEnergy;
}

void
CapacitorEnergySource::SetInitialEnergy (double initialEnergyJ)
{
  NS_LOG_FUNCTION (this << initialEnergyJ);
  NS_ASSERT (initialEnergyJ >= 0);
  double voltage = sqrt (2 * initialEnergyJ / m_capacity);
  if (voltage > m_supplyVoltageV)
    {
      m_initialVoltageV = m_supplyVoltageV;
      m_remainingEnergyJ = m_capacity * pow (m_initialVoltageV, 2) / 2;
    }
  else
    {
      m_remainingEnergyJ = initialEnergyJ;
      m_initialVoltageV = voltage;
    }
  // m_remainingEnergyJ = initialEnergyJ;
  // m_initialVoltageV = voltage;
  m_actualVoltageV = m_initialVoltageV;
  NS_LOG_DEBUG ("Set initial voltage = " << m_initialVoltageV << " V, remaining energy = "
                                         << m_remainingEnergyJ << " J");
}

void
CapacitorEnergySource::SetSupplyVoltage (double supplyVoltageV)
{
  NS_LOG_FUNCTION (this << supplyVoltageV);
  m_supplyVoltageV = supplyVoltageV;
}

void
CapacitorEnergySource::SetUpdateInterval (Time interval)
{
  NS_LOG_FUNCTION (this << interval);
  m_updateInterval = interval;
}

Time
CapacitorEnergySource::GetUpdateInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_updateInterval;
}

double
CapacitorEnergySource::GetSupplyVoltage (void) const
{
  NS_LOG_FUNCTION (this);
  return m_supplyVoltageV;
}


double
CapacitorEnergySource::GetInitialVoltage (void) const
{
  NS_LOG_FUNCTION (this);
  return m_initialVoltageV;
}

double
CapacitorEnergySource::GetInitialEnergy (void) const
{
  NS_LOG_FUNCTION (this);
  return m_capacity*pow(GetInitialVoltage(), 2)/2;
}

double
CapacitorEnergySource::GetRemainingEnergy (void)
{
  NS_LOG_FUNCTION (this);
  // update energy source to get the latest remaining energy.
  UpdateEnergySource ();
  return m_remainingEnergyJ;
}

double
CapacitorEnergySource::GetEnergyFraction (void)
{
  NS_LOG_FUNCTION (this);
  // update energy source to get the latest remaining energy.
  UpdateEnergySource ();
  double initialEnergy = m_capacity*pow(m_initialVoltageV, 2)/2;
  return m_remainingEnergyJ / initialEnergy;
}

double
CapacitorEnergySource::GetVoltageFraction (void)
{
  NS_LOG_FUNCTION (this);

  UpdateEnergySource();
  return m_actualVoltageV / m_initialVoltageV;

}

void
CapacitorEnergySource::UpdateEnergySource (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("CapacitorEnergySource: Updating remaining voltage.");

    double actualVoltage = m_actualVoltageV;
    UpdateVoltage ();

    m_lastUpdateTime = Simulator::Now ();

    NS_LOG_DEBUG("Actual voltage: " << m_actualVoltageV);

    if (!m_depleted && m_actualVoltageV <= m_lowVoltageTh * m_initialVoltageV)
      {
        m_depleted = true;
        HandleEnergyDrainedEvent ();
      }
    else if (m_depleted && m_actualVoltageV > m_highVoltageTh * m_initialVoltageV)
      {
        m_depleted = false;
        HandleEnergyRechargedEvent ();
      }
    else if (m_actualVoltageV != actualVoltage)
      {
        NotifyEnergyChanged ();
      }

    if (m_voltageUpdateEvent.IsExpired ())
      {
        m_voltageUpdateEvent = Simulator::Schedule (m_updateInterval,
                                                  &CapacitorEnergySource::UpdateEnergySource,
                                                 this);
    }
    // Update remaining energy
    m_remainingEnergyJ = m_capacity * pow (m_actualVoltageV, 2) / 2;
    NS_LOG_DEBUG("[DEBUG] Update remaining energy= " << m_remainingEnergyJ);
}

double
CapacitorEnergySource::GetActualVoltage (void)
{
  NS_LOG_FUNCTION (this);
  return m_actualVoltageV;
}


/*
 * Private functions start here.
 */

void
CapacitorEnergySource::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  UpdateEnergySource ();  // start periodic update
}

void
CapacitorEnergySource::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  BreakDeviceEnergyModelRefCycle ();  // break reference cycle
}

void
CapacitorEnergySource::HandleEnergyDrainedEvent (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("CapacitorEnergySource:Energy depleted!");
  NotifyEnergyDrained (); // notify DeviceEnergyModel objects
}

void
CapacitorEnergySource::HandleEnergyRechargedEvent (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("CapacitorEnergySource:Energy recharged!");
  NotifyEnergyRecharged (); // notify DeviceEnergyModel objects
}



// void
// CapacitorEnergySource::CalculateRemainingEnergy (void)
// {
//   NS_LOG_FUNCTION (this);
//   NS_LOG_DEBUG("Old energy value to be updated [J]: " << m_remainingEnergyJ);
//   UpdateVoltage();
//   // energy = current * voltage * time
//   double energyToDecreaseJ = (totalCurrentA * m_actualVoltageV * duration.GetNanoSeconds ()) / 1e9;
//   NS_ASSERT_MSG (m_remainingEnergyJ >= energyToDecreaseJ,
//                  "Remaining energy < energy to decrease "
//                  +std::to_string(energyToDecreaseJ));
//   m_remainingEnergyJ -= energyToDecreaseJ;
//   NS_LOG_DEBUG ("CapacitorEnergySource:Remaining energy = " << m_remainingEnergyJ);
// }

void
CapacitorEnergySource::UpdateVoltage (void)
{
  NS_LOG_FUNCTION (this);
  double Ih = CalculateHarvestersCurrent ();    // current provided by the harvester
  double Iload = CalculateDevicesCurrent (); // Total current draw from all DeviceEnergyModels
  NS_LOG_DEBUG("Harvester Current " << Ih << " Device current " << Iload);
  if (Iload == 0) // Device in OFF state
    {
      NS_LOG_DEBUG("[DEBUG] Device in OFF state, do not update voltage");
      return;
    }
  double Rl = m_supplyVoltageV / Iload;  // Load resistence
  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.IsPositive ());
  double durationS = duration.GetSeconds();
  NS_LOG_DEBUG("Previous voltage: " << m_actualVoltageV <<
               " duration (s) " << durationS <<
               " Rl " << Rl);
  double voltage = Ih*Rl*(1 - exp(-durationS/(Rl * m_capacity))) +
    m_actualVoltageV*exp(-durationS/(Rl*m_capacity));

  NS_LOG_DEBUG ("Actual voltage: " << m_actualVoltageV <<
                " exp= " << exp(-durationS/(Rl*m_capacity)) <<
                " , voltage = " << voltage  );

  if (voltage > m_supplyVoltageV)
    {
      voltage = m_supplyVoltageV;
    }
  m_actualVoltageV = voltage;
}

double
CapacitorEnergySource::CalculateDevicesCurrent (void)
{
  NS_LOG_FUNCTION (this);
  double totalCurrentA = 0.0;
  DeviceEnergyModelContainer::Iterator i;
  for (i = m_models.Begin (); i != m_models.End (); i++)
    {
      totalCurrentA += (*i)->GetCurrentA ();
    }
  return totalCurrentA;
}

double
CapacitorEnergySource::CalculateHarvestersCurrent (void)
{
  NS_LOG_FUNCTION (this);

  double totalHarvestedPower = 0.0;
  std::vector<Ptr<EnergyHarvester>>::const_iterator harvester;
  for (harvester = m_harvesters.begin (); harvester != m_harvesters.end (); harvester++)
    {
      totalHarvestedPower += (*harvester)->GetPower ();
    }

  NS_LOG_DEBUG ("EnergySource(" << GetNode ()->GetId ()
                                << "): Total harvested power = " << totalHarvestedPower);

  double currentHarvestersA;
  if (totalHarvestedPower == 0)
    {
      currentHarvestersA = 0;
    }
  else
    {
      currentHarvestersA = totalHarvestedPower / GetActualVoltage ();
    }

  NS_LOG_DEBUG ("EnergySource(" << GetNode ()->GetId ()
                                << "): Current from harvesters = " << currentHarvestersA);
  return currentHarvestersA;
}


} // namespace ns3
