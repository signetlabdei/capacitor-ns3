
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

#include "ns3/capacitor-energy-source.h"
#include "ns3/lora-radio-energy-model.h"
#include "ns3/variable-energy-harvester.h"
#include "ns3/abort.h"
#include "ns3/device-energy-model.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include "ns3/double.h"
#include "ns3/object-base.h"
#include "ns3/packet.h"
#include "ns3/string.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/type-id.h"
#include <bits/stdint-intn.h>
#include <cmath>
#include <fstream>
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
          .AddAttribute ("Capacitance", "Capacitance of the capacitor [F]",
                         DoubleValue (0.01), // 10 mF
                         MakeDoubleAccessor (&CapacitorEnergySource::m_capacitance),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("RandomInitialVoltage",
                         "Random variable from which taking the initial voltage of the capacitor",
                         StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=0.0]"),
                         MakePointerAccessor (&CapacitorEnergySource::m_initialVoltageRV),
                         MakePointerChecker<RandomVariableStream> ())
          .AddAttribute ("CapacitorMaxSupplyVoltageV",
                         "Max Supply voltage for capacitor energy source.",
                         DoubleValue (3.3), // in Volts
                         MakeDoubleAccessor (&CapacitorEnergySource::m_supplyVoltageV),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("CapacitorLowVoltageThreshold",
                         "Low voltage threshold for capacitor energy source as a fraction of the "
                         "max supply voltage.",
                         DoubleValue (0.0), // as a fraction of the max supply voltage
                         MakeDoubleAccessor (&CapacitorEnergySource::m_lowVoltageTh),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("CapacitorHighVoltageThreshold",
                         "High voltage threshold for capacitor energy source as a fraction of the "
                         "max supply voltage.",
                         DoubleValue (1), // as a fraction of the max supply voltage
                         MakeDoubleAccessor (&CapacitorEnergySource::m_highVoltageTh),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("PeriodicVoltageUpdateInterval",
                         "Time between two consecutive periodic voltage updates.",
                         TimeValue (Seconds (1.0)),
                         MakeTimeAccessor (&CapacitorEnergySource::SetUpdateInterval,
                                           &CapacitorEnergySource::GetUpdateInterval),
                         MakeTimeChecker ())
          .AddAttribute ("FilenameVoltageTracking",
                         "Name of the output file where to save voltage values", StringValue (),
                         MakeStringAccessor (&CapacitorEnergySource::m_filenameVoltageTracking),
                         MakeStringChecker ())
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
  ObjectBase::ConstructSelf(AttributeConstructionList ());
  m_lastUpdateTime = Seconds (0.0);
  m_depleted = false;
  SetInitialVoltage();
}

CapacitorEnergySource::~CapacitorEnergySource ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
CapacitorEnergySource::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

int64_t
CapacitorEnergySource::AssignStreams(int64_t stream)
{
  NS_LOG_FUNCTION (this);
  m_initialVoltageRV -> SetStream(stream);
  return 1;
}

void
CapacitorEnergySource::SetInitialVoltage ()
{
  NS_LOG_FUNCTION (this);

  m_actualVoltageV = m_initialVoltageRV->GetValue ();
  NS_LOG_WARN ("Initial voltage: " << m_actualVoltageV << " V");
  NS_ASSERT(m_actualVoltageV < m_supplyVoltageV);
  m_initialVoltageV = m_actualVoltageV;
  double initialEnergy = m_capacitance*pow(m_actualVoltageV, 2)/2;
  m_remainingEnergyJ = initialEnergy;
  NS_LOG_DEBUG ("Set initial voltage = " << m_actualVoltageV
                << " V, remaining energy = " << initialEnergy);
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
  return m_capacitance*pow(GetInitialVoltage(), 2)/2;
}

double
CapacitorEnergySource::GetRemainingEnergy (void)
{
  NS_LOG_FUNCTION (this);
  // update energy source to get the latest remaining energy.
  if (!(Simulator::Now() == m_lastUpdateTime))
    {
      UpdateEnergySource ();
    }

  NS_LOG_DEBUG(m_remainingEnergyJ);
  return m_remainingEnergyJ;
}

double
CapacitorEnergySource::GetEnergyFraction (void)
{
  NS_LOG_FUNCTION (this);
  // update energy source to get the latest remaining energy.
  UpdateEnergySource ();
  double initialEnergy = m_capacitance*pow(m_initialVoltageV, 2)/2;
  return m_remainingEnergyJ / initialEnergy;
}

double
CapacitorEnergySource::GetActualVoltage (void)
{
  NS_LOG_FUNCTION (this);
  if (Simulator::Now() != m_lastUpdateTime)
  {
    UpdateEnergySource ();
  }

  return m_actualVoltageV;
}

double
CapacitorEnergySource::GetVoltageFraction (void)
{
  NS_LOG_FUNCTION (this);

  UpdateEnergySource ();
  return m_actualVoltageV / m_initialVoltageV;
}

double
CapacitorEnergySource::GetEnergyFromVoltage (double voltage)
{
  NS_LOG_DEBUG (this << " " << voltage);
  double e = m_capacitance * pow (voltage, 2) / 2;
  NS_LOG_DEBUG ("Computed energy " << e);
  return e;
}

bool
CapacitorEnergySource::IsDepleted (void)
{
  NS_LOG_FUNCTION (this);
  return m_depleted;
}

void
CapacitorEnergySource::UpdateEnergySource (void)
{
  NS_LOG_FUNCTION (this);
  // NS_LOG_DEBUG ("CapacitorEnergySource: Updating remaining voltage. Depleted? " << m_depleted);

    double oldVoltage = m_actualVoltageV;
    UpdateVoltage ();

    m_lastUpdateTime = Simulator::Now ();

    double eps = 1e-9;
    // NS_LOG_DEBUG ("Vmin = " << m_lowVoltageTh * m_supplyVoltageV << " actualV "
    //               << (m_actualVoltageV) << " was depleted?" << m_depleted );
      if (!m_depleted && m_actualVoltageV <= m_lowVoltageTh * m_supplyVoltageV + eps)
        {
          NS_LOG_DEBUG ("Energy depleted");
          m_depleted = true;
          HandleEnergyDrainedEvent ();
        }
      else if (m_depleted && m_actualVoltageV > m_highVoltageTh * m_supplyVoltageV)
        {
          NS_LOG_DEBUG ("Energy recharged");
          m_depleted = false;
          HandleEnergyRechargedEvent ();
        }
      else if (m_actualVoltageV != oldVoltage)
        {
          NS_LOG_DEBUG ("Energy changed - new voltage (V)=" << m_actualVoltageV);
          HandleEnergyChangedEvent ();
        }
      else if (m_actualVoltageV == oldVoltage)
        {
          // NS_LOG_DEBUG ("Energy constant ");
          HandleEnergyConstantEvent ();
        }

    if (m_voltageUpdateEvent.IsExpired ())
      {
        m_voltageUpdateEvent = Simulator::Schedule (m_updateInterval,
                                                  &CapacitorEnergySource::UpdateEnergySource,
                                                 this);
    }

    // Track the value (also if it did not change)
    TrackVoltage();

}

double
CapacitorEnergySource::ComputeLoadEnergyConsumption (double Iload, double V0,
                                                     Time duration)
{
  NS_LOG_FUNCTION(this << Iload << duration);

  std::vector<double> r = GetResistances(Iload, GetHarvestersPower());
  double Rload = r.at(0);
  double ri = r.at(1);
  double Req = r.at(2);
  // Define constants
  double A = m_supplyVoltageV*Req/(ri);
  double tau = Req*m_capacitance;
  double t = duration.GetSeconds();

  // Compute the eneergy by integrating the power
  // p(t) = (v(t))^2/Rload
  double energy = 1/Rload * ((pow(A, 2)*t) +
                             0.5 * tau * pow((V0 - A), 2) * (1 - exp(-2*t/tau)) +
                             2 * A * tau * (V0 - A) * (1 - exp(-t/tau)));
  return energy;
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
  NotifyEnergyDrained (); // notify DeviceEnergyModel objects
}

void
CapacitorEnergySource::HandleEnergyRechargedEvent (void)
{
  NS_LOG_FUNCTION (this);
  NotifyEnergyRecharged (); // notify DeviceEnergyModel objects
}

void
CapacitorEnergySource::HandleEnergyChangedEvent (void)
{
  NS_LOG_FUNCTION (this);
  NotifyEnergyChanged (); // notify DeviceEnergyModel objects
}

void
CapacitorEnergySource::HandleEnergyConstantEvent (void)
{
  NS_LOG_FUNCTION (this);
  NotifyEnergyConstant (); // notify DeviceEnergyModel objects
}

void
CapacitorEnergySource::NotifyEnergyConstant (void)
{
  NS_LOG_FUNCTION (this);
  // notify all device energy models installed on node
  DeviceEnergyModelContainer::Iterator i;
  for (i = m_models.Begin (); i != m_models.End (); i++)
    {
      Ptr<lorawan::LoraRadioEnergyModel> loraradio =
          (*i)->GetObject<ns3::lorawan::LoraRadioEnergyModel> ();
      // It is a LoraRadioEnergyModel
      if (!(loraradio == 0))
        {
          TypeId typeId = (*loraradio).GetTypeId ();
          NS_LOG_DEBUG (typeId.GetName ());
          loraradio->ns3::lorawan::LoraRadioEnergyModel::HandleEnergyConstant ();
        }
    }
}

double CapacitorEnergySource::ComputeVoltage (double initialVoltage, double Iload,
                                              double hp, Time duration)
  {
    NS_LOG_FUNCTION (this << " Iload (A): " << Iload << " duration (s): " << duration);
  NS_ASSERT (duration.IsPositive ());

  std::vector<double> r = GetResistances (Iload, GetHarvestersPower ());
  // double Rload = r.at (0);
  double ri = r.at (1);
  double Req = r.at (2);
  double durationS = duration.GetSeconds();
  // NS_LOG_DEBUG("Previous voltage: " << initialVoltage <<
               // " duration (s) " << durationS <<
               // " Rl " << Rload);
  double voltage = m_supplyVoltageV*(Req/ri)*(1 - exp(-durationS/(Req * m_capacitance))) +
    initialVoltage*exp(-durationS/(Req*m_capacitance));

  // NS_LOG_DEBUG ("Previous voltage: " << initialVoltage <<
  //               " exp= " << exp(-durationS/(Rload*m_capacitance)) <<
  //               " , computed voltage = " << voltage  );
  return voltage;
  }

  double
  CapacitorEnergySource::ComputeInitialVoltage (double finalVoltage,
                                                double Iload, double hp,
                                                Time duration)
  {
    NS_LOG_FUNCTION (this << " Iload (A): " << Iload << " duration (s): " << duration);
    std::vector<double> r = GetResistances (Iload, hp);
    double ri = r.at (1);
    double Req = r.at (2);
    NS_LOG_DEBUG("Req= " << Req << " capacitance " << m_capacitance);
    double durationS = duration.GetSeconds();
    double A = m_supplyVoltageV * Req/ri;
    double tau = Req * m_capacitance;
    NS_LOG_DEBUG ("A= " << A << " tau= " << tau);
    double V0 = (1 / exp (-durationS / tau)) *
      (finalVoltage - A * (1 - exp( - durationS/tau))) ;
    NS_LOG_DEBUG ("Computed initial voltage " << V0);
    // Round up voltage to the fourth digit
    V0 = std::ceil(V0*10000)/10000;
    // NS_LOG_DEBUG("finalVoltage = " << finalVoltage);
    NS_LOG_DEBUG ("Computed initial voltage after rounding " << V0);
    return V0;
  }

  void
  CapacitorEnergySource::UpdateVoltage (void)
  {
    NS_LOG_FUNCTION (this);
    Time duration = Simulator::Now () - m_lastUpdateTime;
    double Iload = CalculateDevicesCurrent ();
    double voltage = ComputeVoltage (m_actualVoltageV, Iload, GetHarvestersPower(),
                                     duration);

    m_actualVoltageV = voltage;
    m_lastUpdateTime = Simulator::Now();
    // Update remaining energy
    m_remainingEnergyJ = GetEnergyFromVoltage(voltage);
    NS_LOG_INFO ("Duration: " << duration.GetSeconds() <<
                  " s, new Voltage: " << m_actualVoltageV << " V");
    }

double
CapacitorEnergySource::PredictVoltageForLorawanState (lorawan::EndDeviceLoraPhy::State status,
                                                      double initialVoltage,
                                                      Time duration)
{
  NS_LOG_FUNCTION (this);
  double Iload = 0;
  DeviceEnergyModelContainer container = FindDeviceEnergyModels ("ns3::LoraRadioEnergyModel");
  DeviceEnergyModelContainer::Iterator i;
  for (i = container.Begin (); i != container.End (); i++)
    {
      Ptr<lorawan::LoraRadioEnergyModel> loraradio = (*i)-> GetObject<lorawan::LoraRadioEnergyModel> ();
      if (loraradio == 0)
        {
          NS_LOG_DEBUG("Lora device not found! Returning 0.");
          return 0;
        }
      Iload += loraradio ->GetCurrent (status);
    }
  double voltage = ComputeVoltage(initialVoltage, Iload, GetHarvestersPower(), duration);

return voltage;
}

std::vector<double>
CapacitorEnergySource::GetResistances (double Iload, double hp)
{
  NS_LOG_FUNCTION(this);

  // double Iload = CalculateDevicesCurrent ();
  // double ph = GetHarvestersPower ();
  double ri = pow (m_supplyVoltageV, 2) / hp; // limits the power of the harvesters
  double Rload = 0;
  double Req = 0;
  if (Iload == 0 || hp == 0)
    {
      if (Iload == 0 && !(hp == 0))
        {
          NS_LOG_DEBUG ("[DEBUG] Device in OFF state");
          Req = ri;
        }
      else if (!(Iload == 0) && (hp == 0))
        {
          NS_LOG_DEBUG ("[DEBUG] No harvester");
          Rload = m_supplyVoltageV / Iload; // load resistance
          Req = Rload;
        }
      else
        {
          NS_LOG_ERROR ("No harvested power and no device consumption: the device is in OFF state "
                        "and will never exit!");
          // Assuming no consumption ?
        }
    }
  else
    {
      Rload = m_supplyVoltageV / Iload; // load resistance
      Req = (Rload * ri) / (Rload + ri);
    }
  NS_LOG_DEBUG ("r_i= " << ri << ", Rload= " << Rload << ", Req= " << Req);

  std::vector<double> resistances;
  resistances.push_back(Rload);
  resistances.push_back(ri);
  resistances.push_back(Req);
  return resistances;
}

void
CapacitorEnergySource::SetCheckForEnergyDepletion (void)
{
  NS_LOG_FUNCTION(this);
  double vmin = m_lowVoltageTh *m_supplyVoltageV;
  double Iload = CalculateDevicesCurrent();
  NS_LOG_DEBUG("Iload " << Iload);
  double ph = GetHarvestersPower ();
  double ri = pow (m_supplyVoltageV, 2) / ph; // limits the power of the harvesters
  double Rload = 0;
  double Req = 0;
  if (Iload == 0 || ph == 0)
    {
      if (Iload == 0 && !(ph == 0))
        {
          NS_LOG_DEBUG ("[DEBUG] Device in OFF state");
          Req = ri;
        }
      else if (!(Iload == 0) && (ph == 0))
        {
          NS_LOG_DEBUG ("[DEBUG] No harvester");
          Rload = m_supplyVoltageV / Iload; // load resistance
          Req = Rload;
        }
      else
        {
          NS_LOG_ERROR ("No harvested power and no device consumption: the device is in OFF state "
                        "and will never exit!");
          // Assuming no consumption ?
        }
    }
  else
    {
      Rload = m_supplyVoltageV / Iload; // load resistance
      Req = (Rload * ri) / (Rload + ri);
    }
  double A = m_supplyVoltageV*Req/ri;
  NS_LOG_DEBUG("supplyV " << m_supplyVoltageV << " Req " << Req << " ri " << ri << " Rload " << Rload);
  double t = - Req*m_capacitance*std::log((vmin - A)/
                                     (m_actualVoltageV - A));
  NS_LOG_DEBUG("Delay is [s] " << t);
  NS_LOG_DEBUG ("Actual voltage: " << m_actualVoltageV << " vmin: " << vmin << " A " << A );
  if (!m_checkForEnergyDepletion.IsExpired())
    {
      Simulator::Cancel(m_checkForEnergyDepletion);
    }
  if (t>0)
    {
      Time schedTime = Simulator::Now () + Seconds (t);
      NS_LOG_DEBUG ("Scheduling event at time " << schedTime);
      m_checkForEnergyDepletion =
          Simulator::Schedule (Seconds (t), &CapacitorEnergySource::UpdateEnergySource, this);
    }
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
CapacitorEnergySource::GetHarvestersPower (void)
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

  return totalHarvestedPower;
}

double
CapacitorEnergySource::GetAveragePower (Time time, double samples)
{
  NS_LOG_FUNCTION (this);

  double avgPower = 0.0;
  // Assume single harvester
  Ptr<EnergyHarvester> eh = m_harvesters.at(0);
  NS_LOG_DEBUG ("is not EH " << (eh == 0));
  if (!(eh == 0))
    {
      Ptr<VariableEnergyHarvester> variableEH = eh ->GetObject<VariableEnergyHarvester> ();
      NS_LOG_DEBUG ((variableEH == 0));
      avgPower = variableEH-> GetAveragePower (time, samples);
    }
  // else
  //   {
  //     avgPower = eh-> GetPower();
  //   }
  return avgPower;

}

void
CapacitorEnergySource::TrackVoltage (void)
{
  NS_LOG_FUNCTION (this);
  const char *c = m_filenameVoltageTracking.c_str ();
  std::ofstream outputFile;
  if (Simulator::Now () == Seconds (0))
    {
      // Delete contents of the file as it is opened and put initial V
      outputFile.open (c, std::ofstream::out | std::ofstream::trunc);
    }
  else
    {
      // Append to file
      outputFile.open (c, std::ofstream::out | std::ofstream::app);
    }
  outputFile << Simulator::Now ().GetMilliSeconds () <<
    " " << GetActualVoltage () << std::endl;
  outputFile.close ();
}

std::vector<Ptr<EnergyHarvester>>
CapacitorEnergySource::GetEnergyHarvesters (void)
{
  return m_harvesters; 
}

} // namespace ns3
