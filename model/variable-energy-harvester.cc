
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Wireless Communications and Networking Group (WCNG),
 * University of Rochester, Rochester, NY, USA.
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

#include "variable-energy-harvester.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include <bits/stdint-uintn.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VariableEnergyHarvester");

NS_OBJECT_ENSURE_REGISTERED (VariableEnergyHarvester);

TypeId
VariableEnergyHarvester::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VariableEnergyHarvester")
  .SetParent<EnergyHarvester> ()
  .SetGroupName ("Energy")
  .AddConstructor<VariableEnergyHarvester> ()
  .AddAttribute ("PeriodicHarvestedPowerUpdateInterval",
                 "Time between two consecutive periodic updates of the harvested power. By default, the value is updated every 1 s",
                 TimeValue (Seconds (1.0)),
                 MakeTimeAccessor (&VariableEnergyHarvester::SetHarvestedPowerUpdateInterval,
                                   &VariableEnergyHarvester::GetHarvestedPowerUpdateInterval),
                 MakeTimeChecker ())
    .AddAttribute ("Filename", "Add description",
                   StringValue ("outputixys.csv"),
                   MakeStringAccessor(&VariableEnergyHarvester::SetInputFile),
                   MakeStringChecker())
  .AddTraceSource ("HarvestedPower",
                   "Harvested power by the VariableEnergyHarvester.",
                   MakeTraceSourceAccessor (&VariableEnergyHarvester::m_harvestedPower),
                   "ns3::TracedValueCallback::Double")
  .AddTraceSource ("TotalEnergyHarvested",
                   "Total energy harvested by the harvester.",
                   MakeTraceSourceAccessor (&VariableEnergyHarvester::m_totalEnergyHarvestedJ),
                   "ns3::TracedValueCallback::Double")
  ;
  return tid;
}

VariableEnergyHarvester::VariableEnergyHarvester ()
{
  NS_LOG_FUNCTION (this);
}

VariableEnergyHarvester::VariableEnergyHarvester (Time updateInterval)
{
  NS_LOG_FUNCTION (this << updateInterval);
  m_harvestedPowerUpdateInterval = updateInterval;
}

VariableEnergyHarvester::~VariableEnergyHarvester ()
{
  NS_LOG_FUNCTION (this);
}

  void
  VariableEnergyHarvester::SetInputFile(std::string filename)
  {
    m_filename = filename;
  }

void
VariableEnergyHarvester::SetHarvestedPowerUpdateInterval (Time updateInterval)
{
  NS_LOG_FUNCTION (this << updateInterval);
  m_harvestedPowerUpdateInterval = updateInterval;
}

Time
VariableEnergyHarvester::GetHarvestedPowerUpdateInterval (void) const
{
  NS_LOG_FUNCTION (this);
  return m_harvestedPowerUpdateInterval;
}

/*
 * Private functions start here.
 */

void
VariableEnergyHarvester::UpdateHarvestedPower (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                << "s VariableEnergyHarvester(" << GetNode ()->GetId () << "): Updating harvesting power.");

  Time duration = Simulator::Now () - m_lastHarvestingUpdateTime;

  NS_ASSERT (duration.GetNanoSeconds () >= 0); // check if duration is valid

  double energyHarvested = 0.0;

  // do not update if simulation has finished
  if (Simulator::IsFinished ())
  {
    NS_LOG_DEBUG ("VariableEnergyHarvester: Simulation Finished.");
    return;
  }

  m_energyHarvestingUpdateEvent.Cancel ();

  CalculateHarvestedPower ();

  energyHarvested = duration.GetSeconds () * m_harvestedPower;

  // update total energy harvested
  m_totalEnergyHarvestedJ += energyHarvested;

  // notify energy source
  GetEnergySource ()->UpdateEnergySource ();

  // update last harvesting time stamp
  m_lastHarvestingUpdateTime = Simulator::Now ();

  m_energyHarvestingUpdateEvent = Simulator::Schedule (m_harvestedPowerUpdateInterval,
                                                       &VariableEnergyHarvester::UpdateHarvestedPower,
                                                       this);
}

void
VariableEnergyHarvester::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  ReadPowerFromFile ();

  m_lastHarvestingUpdateTime = Simulator::Now ();

  UpdateHarvestedPower ();  // start periodic harvesting update
}

void
VariableEnergyHarvester::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
}

void
VariableEnergyHarvester::CalculateHarvestedPower (void)
{
  NS_LOG_FUNCTION (this);

  m_harvestedPower = GetPowerFromFile (Simulator::Now());

  NS_LOG_DEBUG (Simulator::Now ().GetSeconds ()
                << "s VariableEnergyHarvester:Harvested energy = " << m_harvestedPower);
}

double
VariableEnergyHarvester::DoGetPower (void) const
{
  NS_LOG_FUNCTION (this);
  return m_harvestedPower;
}
  
void
VariableEnergyHarvester::ReadPowerFromFile ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("Input file: " << m_filename);
  // Assumption: one sample every second

  std::vector<double> power;
  std::string delimiter1 = ",";
  std::ifstream inputfile (m_filename);
  std::string in;

  if (inputfile)
    {
      // Skip the first line
      std::getline (inputfile, in);
      while (std::getline (inputfile, in))
        {
          size_t pos = 0;
          std::vector<std::string> tokens;
          while ((pos = in.find (delimiter1)) != std::string::npos)
            {
              tokens.push_back (in.substr (0, pos));
              in.erase (0, pos + delimiter1.length ());
            }
          tokens.push_back (in.substr (0, pos)); // This to get the last element
          power.push_back(std::stod (tokens[5]));
        }

      inputfile.close ();
    }
  else // input file not found
    {
      power.push_back (0);
      power.push_back (0);
      NS_LOG_DEBUG ("Input file not found!");
    }

  // NS_LOG_DEBUG("power... " << power[0] << " " << power[1]);
  m_power = power;

  }

  double
  VariableEnergyHarvester::GetPowerFromFile (Time time)
  {
    NS_LOG_FUNCTION (this);
    uint8_t t = time.GetSeconds();
    if (m_power.size() > 1)
      {
        NS_ASSERT_MSG(t < m_power.size (), "t " << t << "power_size: " << m_power.size());
        NS_LOG_DEBUG("t: " << double(t) <<" s, Power from file is: " << m_power[t] << " W");
        return m_power[t];
      }
    else
      {
        return 0;
      }

  }


} // namespace ns3
