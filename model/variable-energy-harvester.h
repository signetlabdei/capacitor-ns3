
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

#ifndef BASIC_ENERGY_HARVESTER
#define BASIC_ENERGY_HARVESTER

#include <iostream>

// include from ns-3
#include "ns3/traced-value.h"
#include "ns3/nstime.h"
#include "ns3/event-id.h"
#include "ns3/energy-harvester.h"
#include "ns3/random-variable-stream.h"
#include "ns3/device-energy-model.h"

namespace ns3 {

/**
 * \ingroup energy
 * TODO Add description
 *
 * Unit of power is chosen as Watt since energy models typically calculate
 * energy as (time in seconds * power in Watt).
 *
 */
class VariableEnergyHarvester: public EnergyHarvester
{
public:
  static TypeId GetTypeId (void);

  VariableEnergyHarvester ();

  /**
   * \param updateInterval Energy harvesting update interval.
   *
   * VariableEnergyHarvester constructor function that sets the interval
   * between each update of the value of the power harvested by this
   * energy harvester.
   */
  VariableEnergyHarvester (Time updateInterval);

  virtual ~VariableEnergyHarvester ();

  /**
   * \param updateInterval Energy harvesting update interval.
   *
   * This function sets the interval between each update of the value of the
   * power harvested by this energy harvester.
   */
  void SetHarvestedPowerUpdateInterval (Time updateInterval);

  /**
   * \returns The interval between each update of the harvested power.
   *
   * This function returns the interval between each update of the value of the
   * power harvested by this energy harvester.
   */
  Time GetHarvestedPowerUpdateInterval (void) const;

  /**
   * Set the filename of the file to take as input
   */
  void SetInputFile (std::string filename);


private:
  /// Defined in ns3::Object
  void DoInitialize (void);

  /// Defined in ns3::Object
  void DoDispose (void);

  /**
   * Calculates harvested Power.
   */
  void CalculateHarvestedPower (void);

  /**
   * Read the input file and get the corresponding power 
   */
  void ReadPowerFromFile ();

  // TODO add description
  double GetPowerFromFile (Time time);

  /**
   * \returns m_harvestedPower The power currently provided by the Basic Energy Harvester.
   * Implements DoGetPower defined in EnergyHarvester.
   */
  virtual double DoGetPower (void) const;

  /**
   * This function is called every m_energyHarvestingUpdateInterval in order to
   * update the amount of power that will be provided by the harvester in the
   * next interval.
   */
  void UpdateHarvestedPower (void);

private:

  TracedValue<double> m_harvestedPower;         // current harvested power, in Watt
  TracedValue<double> m_harvestedCurrent;    // current harvested current, in Ampere
  TracedValue<double> m_totalEnergyHarvestedJ;  // total harvested energy, in Joule

  EventId m_energyHarvestingUpdateEvent;        // energy harvesting event
  Time m_lastHarvestingUpdateTime;              // last harvesting time
  Time m_harvestedPowerUpdateInterval;          // harvestable energy update interval

  std::string m_filename;
  std::vector<double> m_power;

};

} // namespace ns3

#endif /* defined(VARIABLE_ENERGY_HARVESTER) */
