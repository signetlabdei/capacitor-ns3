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
 * Authors: Romagnolo Stefano <romagnolostefano93@gmail.com>
 *          Davide Magrin <magrinda@dei.unipd.it>
 *          Martina Capuzzo <capuzzom@dei.unipd.it>
 */

#ifndef LORA_RADIO_ENERGY_MODEL_H
#define LORA_RADIO_ENERGY_MODEL_H

#include "ns3/device-energy-model.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/lora-net-device.h"
#include "ns3/traced-value.h"
#include "end-device-lora-phy.h"
#include "lora-tx-current-model.h"

namespace ns3 {
namespace lorawan {

/**
 * \ingroup energy
 */
class LoraRadioEnergyModelPhyListener : public EndDeviceLoraPhyListener
{
public:
  /**
   * Callback type for updating the transmit current based on the nominal tx power.
   */
  typedef Callback<void, double> UpdateTxCurrentCallback;

  LoraRadioEnergyModelPhyListener ();
  virtual ~LoraRadioEnergyModelPhyListener ();

  /**
   * \brief Sets the change state callback. Used by helper class.
   *
   * \param callback Change state callback.
   */
  void SetChangeStateCallback (DeviceEnergyModel::ChangeStateCallback callback);

  /**
   * \brief Sets the update tx current callback.
   *
   * \param callback Update tx current callback.
   */
  void SetUpdateTxCurrentCallback (UpdateTxCurrentCallback callback);

  /**
   * \brief Switches the LoraRadioEnergyModel to RX state.
   *
   * \param duration the expected duration of the packet reception.
   *
   * Defined in ns3::LoraEndDevicePhyListener
   */
  void NotifyRxStart (void);

  /**
   * \brief Switches the LoraRadioEnergyModel to TX state and switches back to
   * STANDBY after TX duration.
   *
   * \param duration the expected transmission duration.
   * \param txPowerDbm the nominal tx power in dBm
   *
   * Defined in ns3::LoraEndDevicePhyListener
   */
  void NotifyTxStart (double txPowerDbm);

  /**
   * Defined in ns3::LoraEndDevicePhyListener
   */
  void NotifyIdle (void);

  /**
   * Defined in ns3::LoraEndDevicePhyListener
   */
  void NotifySleep (void);

  /**
   * Defined in ns3::LoraEndDevicePhyListener
   */
  void NotifyStandby (void);

  /**
   * Defined in ns3::LoraEndDevicePhyListener
   */
  void NotifyOff (void);

  void NotifyTurnOn (void);

private:
  // /**
  //  * A helper function that makes scheduling m_changeStateCallback possible.
  //  */
  // void SwitchToStandby (void);

  /**
   * Change state callback used to notify the LoraRadioEnergyModel of a state
   * change.
   */
  DeviceEnergyModel::ChangeStateCallback m_changeStateCallback;

  /**
   * Callback used to update the tx current stored in LoraRadioEnergyModel based on
   * the nominal tx power used to transmit the current frame.
   */
  UpdateTxCurrentCallback m_updateTxCurrentCallback;
};


/**
 * \ingroup energy
 * \brief A Lora radio energy model.
 *
 * 4 states are defined for the radio: TX, RX, STANDBY, SLEEP. Default state is
 * STANDBY.
 * The different types of transactions that are defined are:
 *  1. Tx: State goes from STANDBY to TX, radio is in TX state for TX_duration,
 *     then state goes from TX to STANDBY.
 *  2. Rx: State goes from STANDBY to RX, radio is in RX state for RX_duration,
 *     then state goes from RX to STANDBY.
 *  3. Go_to_Sleep: State goes from STANDBY to SLEEP.
 *  4. End_of_Sleep: State goes from SLEEP to STANDBY.
 * The class keeps track of what state the radio is currently in.
 *
 * Energy calculation: For each transaction, this model notifies EnergySource
 * object. The EnergySource object will query this model for the total current.
 * Then the EnergySource object uses the total current to calculate energy.
 *
 */
class LoraRadioEnergyModel : public DeviceEnergyModel
{
public:
  /**
   * Callback type for energy depletion handling.
   */
  typedef Callback<void> LoraRadioEnergyDepletionCallback;

  /**
   * Callback type for energy recharged handling.
   */
  typedef Callback<void> LoraRadioEnergyRechargedCallback;

  /**
   * Callback type for energy changed handling.
   */
  typedef Callback<void, double> LoraRadioEnergyChangedCallback;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  LoraRadioEnergyModel ();
  virtual ~LoraRadioEnergyModel ();

  /**
   * Set the associated LoraNetDevice
   */
  void SetLoraNetDevice (Ptr<LoraNetDevice> device);


  /**
   * \brief Sets pointer to EnergySouce installed on node.
   *
   * \param source Pointer to EnergySource installed on node.
   *
   * Implements DeviceEnergyModel::SetEnergySource.
   */
  void SetEnergySource (Ptr<EnergySource> source);

  /**
   * \returns Total energy consumption of the wifi device.
   *
   * Implements DeviceEnergyModel::GetTotalEnergyConsumption.
   */
  double GetTotalEnergyConsumption (void) const;

  // Setter & getters for state power consumption.
  double GetCurrent (EndDeviceLoraPhy::State status);

  double GetStandbyCurrentA (void) const;
  void SetStandbyCurrentA (double standbyCurrentA);

  double GetTxCurrentA (void) const;
  void SetTxCurrentA (double txCurrentA);

  double GetRxCurrentA (void) const;
  void SetRxCurrentA (double rxCurrentA);

  double GetIdleCurrentA (void) const;
  void SetIdleCurrentA (double idleCurrentA);

  double GetSleepCurrentA (void) const;
  void SetSleepCurrentA (double sleepCurrentA);

  double GetTurnOnCurrentA (void) const;
  void SetTurnOnCurrentA (double turnOnCurrentA);

  /**
   * Compute the load for a status using the reference voltage
   */
  double GetLoraDeviceCurrent (EndDeviceLoraPhy::State status);

  /**
   * \returns Current state.
   */
  EndDeviceLoraPhy::State GetCurrentState (void) const;

  /**
   * \param callback Callback function.
   *
   * Sets callback for energy depletion handling.
   */
  void SetEnergyDepletionCallback (LoraRadioEnergyDepletionCallback callback);

  /**
   * \param callback Callback function.
   *
   * Sets callback for energy recharged handling.
   */
  void SetEnergyRechargedCallback (LoraRadioEnergyRechargedCallback callback);

  void SetEnergyChangedCallback (LoraRadioEnergyChangedCallback callback);


  /**
   * \param model the model used to compute the lora tx current.
   */
  // NOTICE VERY WELL: Current  Model linear or constant as possible choices
  void SetTxCurrentModel (Ptr<LoraTxCurrentModel> model);

  /**
   * \brief Calls the CalcTxCurrent method of the tx current model to
   *        compute the tx current based on such model
   *
   * \param txPowerDbm the nominal tx power in dBm
   */
  // NOTICE VERY WELL: Current  Model linear or constant as possible choices
  void SetTxCurrentFromModel (double txPowerDbm);

  /**
   * \brief Changes state of the LoraRadioEnergyMode.
   *
   * \param newState New state the lora radio is in.
   *
   * Implements DeviceEnergyModel::ChangeState.
   */
  void ChangeState (int newState);

  /**
   * \brief Handles energy depletion.
   *
   * Implements DeviceEnergyModel::HandleEnergyDepletion
   */
  void HandleEnergyDepletion (void);

  /**
   * \brief Handles energy recharged.
   *
   * Implements DeviceEnergyModel::HandleEnergyChanged
   */
  void HandleEnergyChanged (void);

  /**
   * \brief Handles energy recharged.
   *
   * Implements DeviceEnergyModel::HandleEnergyRecharged
   */
  void HandleEnergyRecharged (void);

  /**
   * \returns Pointer to the PHY listener.
   */
  LoraRadioEnergyModelPhyListener * GetPhyListener (void);

  /**
   * Compute the energy consumed by the device in a given state and for a given duration
   */
  double ComputeLoraEnergyConsumption (EndDeviceLoraPhy::State, Time duration);

private:
  void DoDispose (void);

  /**
   * \returns Current draw of device, at current state.
   *
   * Implements DeviceEnergyModel::GetCurrentA.
   */
  double DoGetCurrentA (void) const;

  /**
   * \param state New state the radio device is currently in.
   *
   * Sets current state. This function is private so that only the energy model
   * can change its own state.
   */
  void SetLoraRadioState (const EndDeviceLoraPhy::State state);


  Ptr<EnergySource> m_source; ///< energy source

  // Member variables for current draw in different radio modes.
  double m_txCurrentA; ///< transmit current
  double m_rxCurrentA; ///< receive current
  double m_idleCurrentA; ///< idle current
  double m_standbyCurrentA; ///< standby current
  double m_sleepCurrentA; ///< sleep current
  double m_turnOnCurrentA; // current when turning on
  double m_referenceVoltage; // used with the current to determine the load
  Time m_turnOnDuration;
  double m_sensingEnergy; // Constant energy cost due to sensing before TX
  // NOTICE VERY WELL: Current  Model linear or constant as possible choices
  Ptr<LoraTxCurrentModel> m_txCurrentModel; ///< current model

  /// This variable keeps track of the total energy consumed by this model.
  TracedValue<double> m_totalEnergyConsumption;

  // State variables.
  Ptr<LoraNetDevice> m_device; // A pointer to the device associated to this loranode
  EndDeviceLoraPhy::State m_currentState;  ///< current state the radio is in
  Time m_lastUpdateTime;          ///< time stamp of previous energy update

  uint8_t m_nPendingChangeState; ///< pending state change
  bool m_isSupersededChangeState; ///< superseded change state

  bool m_enterSleepIfDepleted = false; // default: turn off

  /// Energy  callback
  LoraRadioEnergyDepletionCallback m_energyDepletionCallback;
  LoraRadioEnergyRechargedCallback m_energyRechargedCallback;
  LoraRadioEnergyChangedCallback m_energyChangedCallback;

  /// EndDeviceLoraPhy listener
  LoraRadioEnergyModelPhyListener *m_listener;
};

} // namespace ns3

}
#endif /* LORA_RADIO_ENERGY_MODEL_H */
