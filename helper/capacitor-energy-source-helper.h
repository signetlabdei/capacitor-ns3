
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
 * Author: Martina Capuzzo <capuzzom@dei.unipd.it>
 */

#ifndef CAPACITOR_ENERGY_SOURCE_HELPER_H
#define CAPACITOR_ENERGY_SOURCE_HELPER_H

#include "ns3/energy-model-helper.h"
#include "ns3/node.h"

namespace ns3 {

/**
 * \ingroup energy
 * \brief Creates a CapacitorEnergySource object.
 *
 */
class CapacitorEnergySourceHelper : public EnergySourceHelper
{
public:
  CapacitorEnergySourceHelper ();
  ~CapacitorEnergySourceHelper ();

  void Set (std::string name, const AttributeValue &v);

private:
  virtual Ptr<EnergySource> DoInstall (Ptr<Node> node) const;

private:
  ObjectFactory m_capacitorEnergySource;

};

} // namespace ns3

#endif  /* CAPACITOR_ENERGY_SOURCE_HELPER_H */
