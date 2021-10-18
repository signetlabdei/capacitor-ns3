
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

#include "variable-energy-harvester-helper.h"
#include "ns3/energy-harvester.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include "ns3/variable-energy-harvester.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <numeric>
#include <string>
#include <vector>

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("VariableEnergyHarvesterHelper");

VariableEnergyHarvesterHelper::VariableEnergyHarvesterHelper ()
{
  m_variableEnergyHarvester.SetTypeId ("ns3::VariableEnergyHarvester");
}

VariableEnergyHarvesterHelper::~VariableEnergyHarvesterHelper ()
{
}

void
VariableEnergyHarvesterHelper::Set (std::string name, const AttributeValue &v)
{
  m_variableEnergyHarvester.Set (name, v);
}

  // TODO
void
VariableEnergyHarvesterHelper::SetTraces(std::vector<std::string> filenames)
{
  NS_LOG_FUNCTION (this);
  m_filenamesList = filenames;
}

void
VariableEnergyHarvesterHelper::SetVariability (double variability)
{
  NS_LOG_FUNCTION (this);
  m_variability = variability;
}


Ptr<EnergyHarvester>
VariableEnergyHarvesterHelper::DoInstall (Ptr<EnergySource> source) const
{
  NS_ASSERT (source != 0);
  Ptr<Node> node = source->GetNode ();

  // Create a new Variable Energy Harvester
  Ptr<EnergyHarvester> harvester = m_variableEnergyHarvester.Create<VariableEnergyHarvester> ();
  NS_ASSERT (harvester != 0);

  // Connect the Variable Energy Harvester to the Energy Source
  source->ConnectEnergyHarvester (harvester);
  harvester->SetNode (node);
  harvester->SetEnergySource (source);

  // Assign power stream
  std::vector<double> power = ReadPower();
  harvester->GetObject<VariableEnergyHarvester>()-> AssignPowerTrace (power);

  return harvester;
}

  std::vector<double>
  VariableEnergyHarvesterHelper::ReadPower() const
  {
    // This will be the final output
    std::vector<double> combinedPower;

    NS_LOG_DEBUG ("FilenamesList size: " << m_filenamesList.size ());
    // This needed after
    Ptr<UniformRandomVariable> urv;

    // Compute coefficients
    std::vector<double> coefficients (m_filenamesList.size (), 0);
    if (m_filenamesList.size () > 1)
      {
        Ptr<UniformRandomVariable> rv = CreateObject<UniformRandomVariable> ();
        uint w = 0;
        double sumCoefficients = 0;
        double newCoefficient;

        while (sumCoefficients < 1)
          {
            newCoefficient = rv->GetValue (0, 1);
            sumCoefficients = sumCoefficients + newCoefficient;
            NS_LOG_DEBUG ("sum = " << sumCoefficients);
            if ((sumCoefficients < 1) && (w < m_filenamesList.size()))
              {
                coefficients.at (w) = newCoefficient;
                NS_LOG_DEBUG ("Coefficient " << std::to_string (coefficients.at (w)));
                w++;
              }
            else
              {
                NS_LOG_DEBUG("Inside else");
                sumCoefficients = sumCoefficients - newCoefficient;
                // Last element in order to sum to 1
                coefficients.at (m_filenamesList.size () - 1) = 1 - sumCoefficients;
                break;
              }
          }
        // Shuffle them
        std::random_shuffle (coefficients.begin (), coefficients.end ());
        uint m = 0;
        while (m < coefficients.size ())
          {
            NS_LOG_DEBUG ("Shuffled Coefficient " << coefficients.at (m));
            m++;
          }
      }
    else // only one file in the list
      {
        coefficients.at(0) = 1;
        urv = CreateObject<UniformRandomVariable> ();
      }

    // Get the power from the trace
    std::string comma = ",";
    for (uint f=0; f < m_filenamesList.size(); f++)
      {
        std::vector<double> powers;
        NS_LOG_DEBUG ("Input file " << m_filenamesList[f]);
        std::ifstream inputfile (m_filenamesList[f]);
        std::string in;

        // Get power from each file
        if (inputfile)
          {
            double power;
            int sample=0;
            double variability = urv->GetValue (-m_variability, m_variability);
            double thispowervalue;
            NS_LOG_DEBUG ("Variability " << variability);
            while (std::getline (inputfile, in))
              {
                size_t pos = 0;
                pos = in.find (comma);
                if (m_filenamesList.size () > 1)
                  {
                    power = (std::stod (in.substr (pos + comma.length ())));
                  }
                else
                // If we have a single trace, put some variability,
                // so the harvested power differs for each device
                {
                  double readpower = std::stod (in.substr (pos + comma.length ()));
                  NS_LOG_DEBUG ("Read Power value = " << readpower);
                  NS_LOG_DEBUG ("Variability inside loop " << variability);
                  thispowervalue = readpower + variability;
                  NS_LOG_DEBUG ("Thispowervalue " << thispowervalue);
                  power = std::max (double (0), thispowervalue);
                }

                powers.push_back (power);
                NS_LOG_DEBUG ("Sample " << sample << " Power= " << power);
                sample++;
              }
            inputfile.close ();
          }
        else // input file not found
          {
            powers.push_back (0);
            NS_LOG_DEBUG ("Input file not found!");
          }
        // Iteratively compute the linear combination for each sample c
        for (uint c = 0; c < powers.size(); c++)
          {
            if (f == 0) // single input file
              {
                combinedPower.push_back (coefficients.at (f) * powers.at (c));
              }
            else // sum the linear components
              {
                combinedPower.at (c) = combinedPower.at (c) + coefficients.at (f) * powers.at (c);
              }
            // NS_LOG_DEBUG ("Combined power... " << combinedPower.at (c));
          }
      } // end for each input file

  return combinedPower;
  }

} // namespace ns3
