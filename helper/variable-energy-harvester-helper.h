
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

#ifndef VARIABLE_ENERGY_HARVESTER_HELPER_H
#define VARIABLE_ENERGY_HARVESTER_HELPER_H

#include "ns3/variable-energy-harvester.h"
#include "ns3/energy-harvester-helper.h"
#include "ns3/energy-source.h"
#include "ns3/node.h"
#include <vector>

namespace ns3 {

/**
 * \ingroup energy
 * \brief Creates a VariableEnergyHarvester object.
 */
class VariableEnergyHarvesterHelper : public EnergyHarvesterHelper
{
public:
  VariableEnergyHarvesterHelper ();
  ~VariableEnergyHarvesterHelper ();

  void Set (std::string name, const AttributeValue &v);

  void SetVariability (double variability);

  void SetTraces (std::vector<std::string> filenames);

  // Variability when using a single input trace
  double m_variability;

private:
  virtual Ptr<EnergyHarvester> DoInstall (Ptr<EnergySource> source) const;
  std::vector<double> ReadPower (void) const;

private:
  ObjectFactory m_variableEnergyHarvester;
  std::vector<std::string> m_filenamesList;
};

} // namespace ns3

#endif /* defined(VARIABLE_ENERGY_HARVESTER_HELPER_H) */
