/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 *
 * Author: Romagnolo Stefano <romagnolostefano93@gmail.com>
 * Author: Martina Capuzzo <capuzzom@dei.unipd.it>
 */

#include "ns3/abort.h"
#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/capacitor-energy-source.h"
#include "ns3/double.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/end-device-status.h"
#include "ns3/log-macros-enabled.h"
#include "ns3/log.h"
#include "ns3/lora-net-device.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"
#include "ns3/pointer.h"
#include "ns3/energy-source.h"
#include "lora-radio-energy-model.h"
#include "src/core/model/boolean.h"

namespace ns3 {
namespace lorawan {

NS_LOG_COMPONENT_DEFINE ("LoraRadioEnergyModel");

NS_OBJECT_ENSURE_REGISTERED (LoraRadioEnergyModel);

TypeId
LoraRadioEnergyModel::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::LoraRadioEnergyModel")
          .SetParent<DeviceEnergyModel> ()
          .SetGroupName ("Energy")
          .AddConstructor<LoraRadioEnergyModel> ()
          .AddAttribute ("StandbyCurrentA", "The default radio Standby current in Ampere.",
                         DoubleValue (0.0014), // idle mode = 1.4mA
                         MakeDoubleAccessor (&LoraRadioEnergyModel::SetStandbyCurrentA,
                                             &LoraRadioEnergyModel::GetStandbyCurrentA),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("IdleCurrentA", "The default radio Idle current in Ampere.",
                         DoubleValue (0.000007), // idle mode = 7 uA
                         MakeDoubleAccessor (&LoraRadioEnergyModel::SetStandbyCurrentA,
                                             &LoraRadioEnergyModel::GetStandbyCurrentA),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("TxCurrentA", "The radio Tx current in Ampere.",
                         DoubleValue (0.028), // transmit at 0dBm = 28mA
                         MakeDoubleAccessor (&LoraRadioEnergyModel::SetTxCurrentA,
                                             &LoraRadioEnergyModel::GetTxCurrentA),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("RxCurrentA", "The radio Rx current in Ampere.",
                         DoubleValue (0.0112), // receive mode = 11.2mA
                         MakeDoubleAccessor (&LoraRadioEnergyModel::SetRxCurrentA,
                                             &LoraRadioEnergyModel::GetRxCurrentA),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("SleepCurrentA", "The radio Sleep current in Ampere.",
                         DoubleValue (0.0000015), // sleep mode = 1.5microA
                         MakeDoubleAccessor (&LoraRadioEnergyModel::SetSleepCurrentA,
                                             &LoraRadioEnergyModel::GetSleepCurrentA),
                         MakeDoubleChecker<double> ())
          .AddAttribute ("TurnOnCurrentA", "The radio TurnOn current in Ampere.",
                         DoubleValue (0.0221), // turnOn mode = 22.1 mA
                         MakeDoubleAccessor (&LoraRadioEnergyModel::SetTurnOnCurrentA,
                                             &LoraRadioEnergyModel::GetTurnOnCurrentA),
                         MakeDoubleChecker<double> ())
          .AddAttribute (
              "TurnOnDuration", "Amount of time to turn on the device from the OFF state [s]",
              TimeValue (Seconds (13)), MakeTimeAccessor (&LoraRadioEnergyModel::m_turnOnDuration),
              MakeTimeChecker ())
          .AddAttribute ("TxCurrentModel", "A pointer to the attached tx current model.",
                         PointerValue (),
                         MakePointerAccessor (&LoraRadioEnergyModel::m_txCurrentModel),
                         MakePointerChecker<LoraTxCurrentModel> ())
          .AddAttribute ("ReferenceVoltage", "The reference voltage used to compute the load [V]",
                         DoubleValue (3.3),
                         MakeDoubleAccessor (&LoraRadioEnergyModel::m_referenceVoltage),
                         MakeDoubleChecker<double> ())
          .AddAttribute (
              "EnterSleepIfDepleted", "Enter in sleep mode if energy is depleted - else turn off",
              BooleanValue (), MakeBooleanAccessor (&LoraRadioEnergyModel::m_enterSleepIfDepleted),
              MakeBooleanChecker ())
          .AddTraceSource (
              "TotalEnergyConsumption", "Total energy consumption of the radio device.",
              MakeTraceSourceAccessor (&LoraRadioEnergyModel::m_totalEnergyConsumption),
              "ns3::TracedValueCallback::Double")
      // qui potrei aggiungere le callback (e.g., m_changeStateCallback) come
      // tracesource
      ;
  return tid;
}

LoraRadioEnergyModel::LoraRadioEnergyModel ()
{
  NS_LOG_FUNCTION (this);
  m_currentState = EndDeviceLoraPhy::SLEEP;       // initially SLEEP
  m_device = NULL;
  m_lastUpdateTime = Seconds (0.0);
  m_nPendingChangeState = 0;
  m_isSupersededChangeState = false;
  m_energyDepletionCallback.Nullify ();
  m_source = NULL;
  // set callback for EndDeviceLoraPhy listener
  m_listener = new LoraRadioEnergyModelPhyListener;
  m_listener->SetChangeStateCallback (MakeCallback (&DeviceEnergyModel::ChangeState, this));
  // set callback for updating the tx current
  m_listener->SetUpdateTxCurrentCallback (MakeCallback (&LoraRadioEnergyModel::SetTxCurrentFromModel, this));
}

LoraRadioEnergyModel::~LoraRadioEnergyModel ()
{
  NS_LOG_FUNCTION (this);
  delete m_listener;
}


void
LoraRadioEnergyModel::SetLoraNetDevice (Ptr<LoraNetDevice> device)
{
  NS_LOG_FUNCTION (this);

  m_device = device;
}

void
LoraRadioEnergyModel::SetEnergySource (Ptr<EnergySource> source)
{
  NS_LOG_FUNCTION (this << source);
  NS_ASSERT (source != NULL);
  m_source = source;
}

double
LoraRadioEnergyModel::GetTotalEnergyConsumption (void) const
{
  NS_LOG_FUNCTION (this);
  return m_totalEnergyConsumption;
}

double
LoraRadioEnergyModel::GetStandbyCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_idleCurrentA;
}

void
LoraRadioEnergyModel::SetStandbyCurrentA (double idleCurrentA)
{
  NS_LOG_FUNCTION (this << idleCurrentA);
  m_idleCurrentA = idleCurrentA;
}

double
LoraRadioEnergyModel::GetTxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_txCurrentA;
}

void
LoraRadioEnergyModel::SetTxCurrentA (double txCurrentA)
{
  NS_LOG_FUNCTION (this << txCurrentA);
  m_txCurrentA = txCurrentA;
}

double
LoraRadioEnergyModel::GetRxCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxCurrentA;
}

void
LoraRadioEnergyModel::SetRxCurrentA (double rxCurrentA)
{
  NS_LOG_FUNCTION (this << rxCurrentA);
  m_rxCurrentA = rxCurrentA;
}

double
LoraRadioEnergyModel::GetIdleCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_idleCurrentA;
}

void
LoraRadioEnergyModel::SetIdleCurrentA (double idleCurrentA)
{
  NS_LOG_FUNCTION (this << idleCurrentA);
  m_idleCurrentA = idleCurrentA;
}

double
LoraRadioEnergyModel::GetSleepCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_sleepCurrentA;
}

void
LoraRadioEnergyModel::SetSleepCurrentA (double sleepCurrentA)
{
  NS_LOG_FUNCTION (this << sleepCurrentA);
  m_sleepCurrentA = sleepCurrentA;
}

double
LoraRadioEnergyModel::GetTurnOnCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  return m_turnOnCurrentA;
}

void
LoraRadioEnergyModel::SetTurnOnCurrentA (double turnOnCurrentA)
{
  NS_LOG_FUNCTION (this << turnOnCurrentA);
  m_turnOnCurrentA = turnOnCurrentA;
}

    EndDeviceLoraPhy::State
    LoraRadioEnergyModel::GetCurrentState (void) const
{
  NS_LOG_FUNCTION (this);
  Ptr<EndDeviceLoraPhy> edPhy = m_device->GetPhy ()->GetObject<EndDeviceLoraPhy> ();

  NS_ASSERT_MSG (!(edPhy == 0), "EndDeviceLoraPhy object not associated to this LoraRadioEnergyModel!");
  NS_ASSERT_MSG(!(m_currentState == edPhy -> GetState()), "PHY state different from state saved in LoraRadioEnergyModel!");

  return m_currentState;
}


double
LoraRadioEnergyModel::GetCurrent (EndDeviceLoraPhy::State status)
{
  NS_LOG_FUNCTION (status);
  double current;
  switch (status)
    {
    case EndDeviceLoraPhy::STANDBY:
      current =  m_idleCurrentA;
      break;
    case EndDeviceLoraPhy::TX:
      current =  m_txCurrentA;
      break;
    case EndDeviceLoraPhy::RX:
      current =  m_rxCurrentA;
      break;
    case EndDeviceLoraPhy::SLEEP:
      current =  m_sleepCurrentA;
      break;
    case EndDeviceLoraPhy::IDLE:
      current =  m_idleCurrentA;
      break;
    case EndDeviceLoraPhy::OFF:
      current =  0;
      break;
    case EndDeviceLoraPhy::TURNON:
      current = m_turnOnCurrentA;
      break;
    default:
      NS_FATAL_ERROR ("LoraRadioEnergyModel:Undefined radio state:" << status);
    }
  return current;
}


// Callbacks
void
LoraRadioEnergyModel::SetEnergyDepletionCallback (
  LoraRadioEnergyDepletionCallback callback)
{
  NS_LOG_FUNCTION (this);
  if (callback.IsNull ())
    {
      NS_LOG_DEBUG ("LoraRadioEnergyModel:Setting NULL energy depletion callback!");
    }
  m_energyDepletionCallback = callback;
}

void
LoraRadioEnergyModel::SetEnergyRechargedCallback (
  LoraRadioEnergyRechargedCallback callback)
{
  NS_LOG_FUNCTION (this);
  if (callback.IsNull ())
    {
      NS_LOG_DEBUG ("LoraRadioEnergyModel:Setting NULL energy recharged callback!");
    }
  m_energyRechargedCallback = callback;
}

void
LoraRadioEnergyModel::SetTxCurrentModel (Ptr<LoraTxCurrentModel> model)
{
  m_txCurrentModel = model;
}

void
LoraRadioEnergyModel::SetTxCurrentFromModel (double txPowerDbm)
{
  if (m_txCurrentModel)
    {
      m_txCurrentA = m_txCurrentModel->CalcTxCurrent (txPowerDbm);
    }
}

void
LoraRadioEnergyModel::ChangeState (int newState)
{
  NS_LOG_FUNCTION (this << "Changing state... Computing energy consumption for the previous state");

  Time duration = Simulator::Now () - m_lastUpdateTime;
  NS_ASSERT (duration.GetNanoSeconds () >= 0);     // check if duration is valid

  // energy to decrease = current * voltage * time
  double energyToDecrease = 0.0;

  energyToDecrease = ComputeLoraEnergyConsumption (m_currentState, duration);

  // update total energy consumption
  m_totalEnergyConsumption += energyToDecrease;

  // update last update time stamp
  m_lastUpdateTime = Simulator::Now ();

  m_nPendingChangeState++;

  // notify energy source
  m_source->UpdateEnergySource ();

  // in case the energy source is found to be depleted during the last update, a callback might be
  // invoked that might cause a change in the Lora PHY state (e.g., the PHY is put into SLEEP mode).
  // This in turn causes a new call to this member function, with the consequence that the previous
  // instance is resumed after the termination of the new instance. In particular, the state set
  // by the previous instance is erroneously the final state stored in m_currentState. The check below
  // ensures that previous instances do not change m_currentState.

  if (!m_isSupersededChangeState)
    {
      // update current state & last update time stamp
      SetLoraRadioState ((EndDeviceLoraPhy::State) newState);
      NS_LOG_DEBUG("[DEBUG] Set a new state");

      // // Also update PHY
      // Ptr<EndDeviceLoraPhy> edPhy = m_device->GetPhy ()->GetObject<EndDeviceLoraPhy> ();
      // switch (newState)
      // {
      // case EndDeviceLoraPhy::STANDBY:
      //   // This can not be done by this class
      //   // TODO Implement for Class A devices
      //   break;
      // case EndDeviceLoraPhy::TX:
      //   // This can not be done by this class
      //   break;
      // case EndDeviceLoraPhy::RX:
      //   // This can not be done by this class
      //   break;
      // case EndDeviceLoraPhy::SLEEP:
      //   edPhy->SwitchToSleep ();
      //   break;
      // case EndDeviceLoraPhy::OFF:
      //   edPhy->SwitchToOff ();
      //   break;
      // default:
      //   NS_FATAL_ERROR ("LoraRadioEnergyModel:Undefined radio state: " << newState);
      //   }

    }

  m_isSupersededChangeState = (m_nPendingChangeState > 1);

  m_nPendingChangeState--;
}

void
LoraRadioEnergyModel::HandleEnergyDepletion (void)
{
  NS_LOG_FUNCTION (this);

  // TODO Take decision with separate function to have dynamic choice

  if (m_enterSleepIfDepleted)
    {
      NS_LOG_DEBUG ("LoraRadioEnergyModel:Energy is depleted! Switching to SLEEP mode");
      // ChangeState(EndDeviceLoraPhy::OFF); // This will be done by the notification
      Ptr<EndDeviceLoraPhy> edPhy = m_device->GetPhy ()->GetObject<EndDeviceLoraPhy> ();
      edPhy->SwitchToSleep ();
    }
  else
    {
      NS_LOG_DEBUG ("LoraRadioEnergyModel:Energy is depleted! Switching to OFF mode");
      // ChangeState(EndDeviceLoraPhy::OFF); // This will be done by the notification
      Ptr<EndDeviceLoraPhy> edPhy = m_device->GetPhy ()->GetObject<EndDeviceLoraPhy> ();
      edPhy->SwitchToOff ();
    }

  // TODO insert event
  // invoke energy depletion callback, if set.
  if (!m_energyDepletionCallback.IsNull ())
    {
      m_energyDepletionCallback ();
    }
}

void
LoraRadioEnergyModel::HandleEnergyChanged (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LoraRadioEnergyModel:Energy changed!");
}

void
LoraRadioEnergyModel::HandleEnergyRecharged (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("LoraRadioEnergyModel:Energy is recharged! Turning on the ED");

  // This may have a cost
  m_source -> UpdateEnergySource();

  // ChangeState (EndDeviceLoraPhy::SLEEP);
  Ptr<EndDeviceLoraPhy> edPhy = m_device->GetPhy ()->GetObject<EndDeviceLoraPhy> ();
  if (edPhy->GetState () == EndDeviceLoraPhy::OFF)
    {
      edPhy->SwitchToTurnOn ();
      Simulator::Schedule (m_turnOnDuration, &EndDeviceLoraPhy::SwitchToSleep, edPhy);
    }
  else
    {
      edPhy->SwitchToSleep ();
    }

  // TODO insert event
  NS_LOG_DEBUG("TODO: Update tracker");
  // invoke energy recharged callback, if set.
  if (!m_energyRechargedCallback.IsNull ())
    {
      NS_LOG_DEBUG("Calling the callback...");
      m_energyRechargedCallback ();
    }
}

LoraRadioEnergyModelPhyListener *
LoraRadioEnergyModel::GetPhyListener (void)
{
  NS_LOG_FUNCTION (this);
  return m_listener;
}

/*
 * Private functions start here.
 */

void
LoraRadioEnergyModel::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_source = NULL;
  m_energyDepletionCallback.Nullify ();
}

double
LoraRadioEnergyModel::DoGetCurrentA (void) const
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG("[DEBUG] current state: "<< m_currentState );
  switch (m_currentState)
    {
    case EndDeviceLoraPhy::STANDBY:
      return m_idleCurrentA;
    case EndDeviceLoraPhy::TX:
      return m_txCurrentA;
    case EndDeviceLoraPhy::RX:
      return m_rxCurrentA;
    case EndDeviceLoraPhy::SLEEP:
      return m_sleepCurrentA;
    case EndDeviceLoraPhy::IDLE:
      return m_idleCurrentA;
    case EndDeviceLoraPhy::OFF:
      return 0;
    case EndDeviceLoraPhy::TURNON:
      return m_turnOnCurrentA;
    default:
      NS_FATAL_ERROR ("LoraRadioEnergyModel:Undefined radio state:" << m_currentState);
    }
}

void
LoraRadioEnergyModel::SetLoraRadioState (const EndDeviceLoraPhy::State state)
{
  NS_LOG_FUNCTION (this << state);
  m_currentState = state;
  std::string stateName;
  switch (state)
    {
    case EndDeviceLoraPhy::STANDBY:
      stateName = "STANDBY";
      break;
    case EndDeviceLoraPhy::TX:
      stateName = "TX";
      break;
    case EndDeviceLoraPhy::RX:
      stateName = "RX";
      break;
    case EndDeviceLoraPhy::IDLE:
      stateName = "IDLE";
      break;
    case EndDeviceLoraPhy::SLEEP:
      stateName = "SLEEP";
      break;
    case EndDeviceLoraPhy::OFF:
      stateName = "OFF";
    case EndDeviceLoraPhy::TURNON:
      stateName = "TURNON";
    }
  NS_LOG_DEBUG ("LoraRadioEnergyModel:Switching to state: " << stateName <<
                " at time = " << Simulator::Now ().GetSeconds () << " s");
}

double
LoraRadioEnergyModel::ComputeLoraEnergyConsumption (EndDeviceLoraPhy::State status,
                                                    Time duration)
{
  NS_LOG_FUNCTION(this);
  NS_LOG_DEBUG("State: " << status << " duration (s): " << duration.GetSeconds());

  double energy = 0;
  double voltage;
  Ptr<CapacitorEnergySource> capacitor = m_source -> GetObject<CapacitorEnergySource>();
  if (!(capacitor == 0))
    {
      voltage =
          capacitor->PredictVoltageForLoraState (status, duration);
      NS_LOG_DEBUG("New voltage [V] = " << voltage );
    }
  else
      {
        // TODO Check this...
        NS_ABORT_MSG("Check energy source");
        voltage = m_source->GetSupplyVoltage ();
      }

  double load = m_referenceVoltage/GetCurrent (status);
  double current = voltage/load;
  NS_LOG_DEBUG ("load = " << load);

  // The following is the same as E = P t, with P = (v(t))^2/Rload
  energy = duration.GetSeconds () * current * voltage;

  NS_LOG_DEBUG("new energy = " << energy);
  return energy;
}


// -------------------------------------------------------------------------- //

LoraRadioEnergyModelPhyListener::LoraRadioEnergyModelPhyListener ()
{
  NS_LOG_FUNCTION (this);
  m_changeStateCallback.Nullify ();
  m_updateTxCurrentCallback.Nullify ();
}

LoraRadioEnergyModelPhyListener::~LoraRadioEnergyModelPhyListener ()
{
  NS_LOG_FUNCTION (this);
}

void
LoraRadioEnergyModelPhyListener::SetChangeStateCallback (DeviceEnergyModel::ChangeStateCallback callback)
{
  NS_LOG_FUNCTION (this << &callback);
  NS_ASSERT (!callback.IsNull ());
  m_changeStateCallback = callback;
}

void
LoraRadioEnergyModelPhyListener::SetUpdateTxCurrentCallback (UpdateTxCurrentCallback callback)
{
  NS_LOG_FUNCTION (this << &callback);
  NS_ASSERT (!callback.IsNull ());
  m_updateTxCurrentCallback = callback;
}

void
LoraRadioEnergyModelPhyListener::NotifyRxStart ()
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LoraRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (EndDeviceLoraPhy::RX);
}

void
LoraRadioEnergyModelPhyListener::NotifyTxStart (double txPowerDbm)
{
  NS_LOG_FUNCTION (this << txPowerDbm);
  if (m_updateTxCurrentCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LoraRadioEnergyModelPhyListener:Update tx current callback not set!");
    }
  m_updateTxCurrentCallback (txPowerDbm);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LoraRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (EndDeviceLoraPhy::TX);
  NS_LOG_DEBUG("ChangeState callback called");
}

void
LoraRadioEnergyModelPhyListener::NotifyIdle (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LoraRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (EndDeviceLoraPhy::IDLE);
}

void
LoraRadioEnergyModelPhyListener::NotifySleep (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LoraRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (EndDeviceLoraPhy::SLEEP);
}

void
LoraRadioEnergyModelPhyListener::NotifyStandby (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LoraRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (EndDeviceLoraPhy::STANDBY);
}

void
LoraRadioEnergyModelPhyListener::NotifyOff (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LoraRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (EndDeviceLoraPhy::OFF);
}

void
LoraRadioEnergyModelPhyListener::NotifyTurnOn (void)
{
  NS_LOG_FUNCTION (this);
  if (m_changeStateCallback.IsNull ())
    {
      NS_FATAL_ERROR ("LoraRadioEnergyModelPhyListener:Change state callback not set!");
    }
  m_changeStateCallback (EndDeviceLoraPhy::TURNON);
}

/*
 * Private function state here.
 */

// void
// LoraRadioEnergyModelPhyListener::SwitchToStandby (void)
// {
//   NS_LOG_FUNCTION (this);
//   if (m_changeStateCallback.IsNull ())
//     {
//       NS_FATAL_ERROR ("LoraRadioEnergyModelPhyListener:Change state callback not set!");
//     }
//   m_changeStateCallback (EndDeviceLoraPhy::STANDBY);
// }

} // namespace lorawan
} // namespace ns3
