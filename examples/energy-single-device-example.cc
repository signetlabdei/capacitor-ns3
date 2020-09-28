
/*
 * This script simulates a simple network to explain how the Lora energy model
 * works.
 */

#include "ns3/basic-energy-harvester.h"
#include "ns3/callback.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/end-device-lorawan-mac.h"
#include "ns3/energy-harvester-container.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/class-a-end-device-lorawan-mac.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/lora-net-device.h"
#include "ns3/lora-radio-energy-model.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/position-allocator.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/command-line.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/basic-energy-harvester-helper.h"
#include "ns3/lora-radio-energy-model-helper.h"
#include "ns3/file-helper.h"
#include "ns3/names.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include <algorithm>
#include <ctime>
#include <iostream>
#include <string>
#include <sys/types.h>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE ("EnergySingleDeviceExample");

// Trace sinks

void
OnRemainingEnergyChange (double oldRemainingEnergy, double remainingEnergy)
{
  // NS_LOG_DEBUG (Simulator::Now().GetSeconds() << " " << remainingEnergy);
  std::cout << Simulator::Now().GetSeconds() << " " << remainingEnergy << std::endl;
  // return std::to_string(Simulator::Now().GetSeconds()) + " " + std::to_string(remainingEnergy)
}

void OnEnergyHarvested (double oldvalue, double totalEnergyHarvested)
{
  NS_LOG_DEBUG("Total energy harvested: " << totalEnergyHarvested);
  NS_LOG_DEBUG ("Energy harvested in this interval: " << totalEnergyHarvested - oldvalue);
}

void
EnergyDepletionCallback (void)
{
  NS_LOG_DEBUG("Energy depleted callback in main");
}

std::string
CheckEnoughEnergyCallback (uint32_t nodeId, Ptr<const Packet> packet, Time time, bool boolValue)
{
  return std::string(std::to_string(time.GetSeconds()) + " " + std::to_string(boolValue));
}
// void
// CheckEnoughEnergyCallback (uint32_t nodeId, Ptr<const Packet> packet, Time time, bool boolValue)
// {
//   NS_LOG_DEBUG("Enough energy to tx? " << boolValue);
// }

int main (int argc, char *argv[])
{

  // Set up logging
  LogComponentEnable ("EnergySingleDeviceExample", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraRadioEnergyModel", LOG_LEVEL_ALL);
  // LogComponentEnable ("EnergyHarvester", LOG_LEVEL_ALL);
  // LogComponentEnable ("EnergySource", LOG_LEVEL_ALL);
  // LogComponentEnable ("BasicEnergySource", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraChannel", LOG_LEVEL_INFO);
  // LogComponentEnable ("LoraPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("SimpleEndDeviceLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable ("GatewayLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraInterferenceHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LorawanMac", LOG_LEVEL_ALL);
  // LogComponentEnable ("EndDeviceLorawanMac", LOG_LEVEL_ALL);
  LogComponentEnable ("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
  // LogComponentEnable ("GatewayLorawanMac", LOG_LEVEL_ALL);
  // LogComponentEnable ("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LogicalLoraChannel", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraPhyHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LorawanMacHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("OneShotSenderHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("OneShotSender", LOG_LEVEL_ALL);
  // LogComponentEnable ("LorawanMacHeader", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraFrameHeader", LOG_LEVEL_ALL);

  LogComponentEnableAll (LOG_PREFIX_FUNC);
  // LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);

  /************************
  *  Create the channel  *
  ************************/

  // NS_LOG_INFO ("Creating the channel...");

  // Create the lora channel object
  Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
  loss->SetPathLossExponent (3.76);
  loss->SetReference (1, 7.7);

  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

  Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

  /************************
  *  Create the helpers  *
  ************************/

  // NS_LOG_INFO ("Setting up helpers...");

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
  allocator->Add (Vector (100,0,0));
  allocator->Add (Vector (0,0,0));
  mobility.SetPositionAllocator (allocator);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  // Create the LoraPhyHelper
  LoraPhyHelper phyHelper = LoraPhyHelper ();
  phyHelper.SetChannel (channel);

  // Create the LorawanMacHelper
  LorawanMacHelper macHelper = LorawanMacHelper ();

  // Create the LoraHelper
  LoraHelper helper = LoraHelper ();

  /************************
  *  Create End Devices  *
  ************************/

  // NS_LOG_INFO ("Creating the end device...");

  // Create a set of nodes
  NodeContainer endDevices;
  endDevices.Create (1);

  // Assign a mobility model to the node
  mobility.Install (endDevices);

  // Create the LoraNetDevices of the end devices
  phyHelper.SetDeviceType (LoraPhyHelper::ED);
  macHelper.SetDeviceType (LorawanMacHelper::ED_A);
  NetDeviceContainer endDevicesNetDevices = helper.Install (phyHelper, macHelper, endDevices);

  /*********************
   *  Create Gateways  *
   *********************/

  // NS_LOG_INFO ("Creating the gateway...");
  NodeContainer gateways;
  gateways.Create (1);

  mobility.SetPositionAllocator (allocator);
  mobility.Install (gateways);

  // Create a netdevice for each gateway
  phyHelper.SetDeviceType (LoraPhyHelper::GW);
  macHelper.SetDeviceType (LorawanMacHelper::GW);
  helper.Install (phyHelper, macHelper, gateways);

  macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);

  /*********************************************
   *  Install applications on the end devices  *
   *********************************************/

  PeriodicSenderHelper periodicSenderHelper;
  periodicSenderHelper.SetPeriod (Seconds (15));

  periodicSenderHelper.Install (endDevices);

  /************************
   * Install Energy Model *
   ************************/

  BasicEnergySourceHelper basicSourceHelper;
  LoraRadioEnergyModelHelper radioEnergyHelper;

  // configure energy source
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (1)); // Energy in J
  basicSourceHelper.Set ("BasicEnergySupplyVoltageV", DoubleValue (3.3));
  basicSourceHelper.Set ("BasicEnergyLowBatteryThreshold", DoubleValue (0.1));
  basicSourceHelper.Set ("BasicEnergyHighBatteryThreshold", DoubleValue (0.3));
  basicSourceHelper.Set ("PeriodicEnergyUpdateInterval", TimeValue(MilliSeconds(20)));

  radioEnergyHelper.Set ("StandbyCurrentA", DoubleValue (0.0014));
  // radioEnergyHelper.Set ("StandbyCurrentA", DoubleValue (0.01));
  radioEnergyHelper.Set ("SleepCurrentA", DoubleValue (0.0000015));
  // radioEnergyHelper.Set ("SleepCurrentA", DoubleValue (0.002));
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.0112));

  radioEnergyHelper.SetTxCurrentModel ("ns3::ConstantLoraTxCurrentModel",
                                       "TxCurrent", DoubleValue (0.028));
  // radioEnergyHelper.SetEnergyDepletionCallback(MakeCallback(&EnergyDepletionCallback));

 //  // Energy harvesting
  BasicEnergyHarvesterHelper harvesterHelper;
  harvesterHelper.Set ("PeriodicHarvestedPowerUpdateInterval",
                       TimeValue (Seconds(10)));
  harvesterHelper.Set ("HarvestablePower",
                      StringValue ("ns3::UniformRandomVariable[Min=0|Max=0.001]"));

  // install source on EDs' nodes
  EnergySourceContainer sources = basicSourceHelper.Install (endDevices);
  Names::Add ("/Names/EnergySource", sources.Get (0));

  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install
      (endDevicesNetDevices, sources);

  // install harvester on the energy source
  EnergyHarvesterContainer harvesters = harvesterHelper.Install (sources);

  Names::Add("Names/EnergyHarvester", harvesters.Get (0));
  Ptr<EnergyHarvester> myHarvester = harvesters.Get(0);
  // myHarvester -> TraceConnectWithoutContext("TotalEnergyHarvested",
  //                                           MakeCallback(&OnEnergyHarvested));


  // Names::Add("Names/myEDmac", endDevices.Get(0)->GetObject<EndDeviceLorawanMac>());
  Ptr<EndDeviceLorawanMac> myEDmac = endDevices.Get (0)->GetDevice(0)->GetObject<LoraNetDevice>()->GetMac ()->GetObject<EndDeviceLorawanMac>();
  // myEDmac->TraceConnectWithoutContext ("EnoughEnergyToTx", MakeCallback(&CheckEnoughEnergyCallback));

  ns3::Config::ConnectWithoutContext ("/Names/EnergySource/RemainingEnergy",
                                          MakeCallback (&OnRemainingEnergyChange));

  /****************
  *  Simulation  *
  ****************/

  Simulator::Stop (Seconds (50));

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}
