
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

#include "capacitor-energy-source-helper.h"
#include "ns3/energy-source.h"
#include "ns3/log.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("CapacitorEnergySourceHelper");

CapacitorEnergySourceHelper::CapacitorEnergySourceHelper ()
{
  m_capacitorEnergySource.SetTypeId ("ns3::CapacitorEnergySource");
}

CapacitorEnergySourceHelper::~CapacitorEnergySourceHelper ()
{
}

void
CapacitorEnergySourceHelper::Set (std::string name, const AttributeValue &v)
{
  m_capacitorEnergySource.Set (name, v);
}

Ptr<EnergySource>
CapacitorEnergySourceHelper::DoInstall (Ptr<Node> node) const
{
  NS_ASSERT (node != NULL);
  NS_LOG_DEBUG("Installing capacitor on node " << node->GetId());
  Ptr<EnergySource> source = m_capacitorEnergySource.Create<EnergySource> ();
  NS_ASSERT (source != NULL);
  source->SetNode (node);
  return source;
}

} // namespace ns3
