 /*
 * Script to compare with the model: Delgado, battery-less, IoTJ
 */

#include "ns3/basic-energy-harvester.h"
#include "ns3/boolean.h"
#include "ns3/callback.h"
#include "ns3/double.h"
#include "ns3/end-device-lora-phy.h"
#include "ns3/end-device-lorawan-mac.h"
#include "ns3/energy-harvester-container.h"
#include "ns3/gateway-lora-phy.h"
#include "ns3/class-a-end-device-lorawan-mac.h"
#include "ns3/gateway-lorawan-mac.h"
#include "ns3/integer.h"
#include "ns3/lora-net-device.h"
#include "ns3/lora-radio-energy-model.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/one-shot-sender-helper.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/lora-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/position-allocator.h"
#include "ns3/periodic-sender-helper.h"
#include "ns3/energy-aware-sender-helper.h"
#include "ns3/command-line.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/basic-energy-harvester-helper.h"
#include "ns3/lora-radio-energy-model-helper.h"
#include "ns3/network-server-helper.h"
#include "ns3/forwarder-helper.h"
#include "ns3/file-helper.h"
#include "ns3/names.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "src/core/model/config.h"
#include "src/core/model/log.h"
#include "src/lorawan/helper/variable-energy-harvester-helper.h"
#include "src/lorawan/model/capacitor-energy-source.h"
#include "src/lorawan/helper/capacitor-energy-source-helper.h"
#include "src/lorawan/model/lora-tx-current-model.h"
#include <algorithm>
#include <bits/stdint-uintn.h>
#include <ctime>
#include <iostream>
#include <string>
#include <sys/types.h>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE ("ModelComparisonEnergy");

// Inputs
double capacity = 6;
double simTime = 5;
double appPeriod = 10;
int packetSize = 10;
double eh = 0.001;
uint8_t dr = 5;
bool confirmed = false;
bool energyAwareSender = false;
bool enableVariableHarvester = false;
bool sun = true;
std::string filenameEnergyConsumption = "energyConsumption.txt";
std::string filenameRemainingEnergy = "remainingEnergy.txt";
std::string filenameState = "deviceStates.txt";
std::string filenameEnoughEnergy = "energyEnoughForTx.txt";
std::string pathToInputFile = "/home/marty/work/ua/panels_data";
std::string filenameHarvesterSun = "/outputixys.csv";
std::string filenameHarvesterCloudy = "/outputixys_cloudy.csv";
bool receivedDLCallbackFirstCall = true;
bool receivedULCallbackFirstCall = true;
bool ULCycleCompleted = false;

    // Callbcks
    void
    OnCloseRX2Callback (void)
{
  // NS_LOG_DEBUG ("Called an empty callback");
  std::cout << "0 1" << std::endl;
  if (!ULCycleCompleted)
    {
      ULCycleCompleted = true;
    }
}

void
OnReceivedULPacketCallback (Ptr<Packet const> packet)
{
  // NS_LOG_DEBUG (Simulator::Now().GetSeconds() << " " << remainingEnergy);
  std::cout << "1 1" << std::endl;
  if (receivedULCallbackFirstCall)
    {
      receivedULCallbackFirstCall = false;
    }
}

void
OnReceivedDLPacketCallback (Ptr<Packet const> packet)
{
  // NS_LOG_DEBUG (Simulator::Now().GetSeconds() << " " << remainingEnergy);
  std::cout << "2 1" << std::endl;
  if (receivedDLCallbackFirstCall)
    {
      receivedDLCallbackFirstCall = false;
    }
}


/***********
 ** MAIN  **
 **********/
int main (int argc, char *argv[])
{

  // Inputs
  CommandLine cmd;
  // cmd.AddValue ("pathToInputFile",
  //               "Absolute path till the input file for the varaible energy harvester",
  //               pathToInputFile);
  cmd.AddValue ("capacity", "Capacity in mF", capacity);
  cmd.AddValue ("packetSize", "PacketSize", packetSize);
  cmd.AddValue ("dr", "dr", dr);
  cmd.AddValue ("confirmed", "confirmed", confirmed);
  cmd.AddValue ("eh", "eh", eh);
  // cmd.AddValue ("simTime", "Simulation time [s]", simTime);
  // cmd.AddValue ("appPeriod", "App period [s]", appPeriod);
  // cmd.AddValue ("energyAwareSender", "Send packet when energy allows it", energyAwareSender);
  // cmd.AddValue ("EnableVariableHarvester", "Enable harvester from input file",
  //                enableVariableHarvester);
  // cmd.AddValue ("sun", "Input from sunny day", sun);
  cmd.Parse (argc, argv);

  // Set up logging
  LogComponentEnable ("ModelComparisonEnergy", LOG_LEVEL_ALL);
  LogComponentEnable ("CapacitorEnergySource", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraRadioEnergyModel", LOG_LEVEL_ALL);
  // LogComponentEnable ("EnergyHarvester", LOG_LEVEL_ALL);
  // LogComponentEnable ("VariableEnergyHarvester", LOG_LEVEL_ALL);
  // LogComponentEnable ("EnergySource", LOG_LEVEL_ALL);
  // LogComponentEnable ("BasicEnergySource", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraChannel", LOG_LEVEL_ALL);
  LogComponentEnable ("LoraPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("EndDeviceLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable ("SimpleEndDeviceLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable ("SimpleGatewayLoraPhy", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraInterferenceHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LorawanMac", LOG_LEVEL_ALL);
  // LogComponentEnable ("EndDeviceLorawanMac", LOG_LEVEL_ALL);
  LogComponentEnable ("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
  // LogComponentEnable ("GatewayLorawanMac", LOG_LEVEL_ALL);
  // LogComponentEnable ("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LogicalLoraChannel", LOG_LEVEL_ALL);
  // LogComponentEnable("NetworkStatus", LOG_LEVEL_ALL);
  // LogComponentEnable ("EndDeviceStatus", LOG_LEVEL_ALL);
  // LogComponentEnable ("NetworkScheduler", LOG_LEVEL_ALL);
  // LogComponentEnable ("NetworkServer", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraPhyHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LorawanMacHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("OneShotSenderHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("OneShotSender", LOG_LEVEL_ALL);
  // LogComponentEnable ("PeriodicSender", LOG_LEVEL_ALL);
  // LogComponentEnable ("EnergyAwareSender", LOG_LEVEL_ALL);
  // LogComponentEnable ("EnergyAwareSenderHelper", LOG_LEVEL_ALL);
  // LogComponentEnable ("LorawanMacHeader", LOG_LEVEL_ALL);
  // LogComponentEnable ("LoraFrameHeader", LOG_LEVEL_ALL);

  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnableAll (LOG_PREFIX_TIME);

  // Set SF
  Config::SetDefault ("ns3::EndDeviceLorawanMac::DataRate", UintegerValue (dr));

  // Select input file
  std::string filenameHarvester;
  if (sun)
    {
      filenameHarvester = pathToInputFile + filenameHarvesterSun;
    }
  else
    {
      filenameHarvester = pathToInputFile + filenameHarvesterCloudy;
    }
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

  // macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);

  // Set message type (Default is unconfirmed)
  Ptr<LorawanMac> edMac1 =
      endDevices.Get (0)->GetDevice (0)->GetObject<LoraNetDevice> ()->GetMac ();
  Ptr<ClassAEndDeviceLorawanMac> edLorawanMac1 = edMac1->GetObject<ClassAEndDeviceLorawanMac> ();
  if (confirmed)
    {
      edLorawanMac1->SetMType (LorawanMacHeader::CONFIRMED_DATA_UP);
    }

  /*********************************************
   *  Install applications on the end devices  *
   *********************************************/

  if (energyAwareSender)
    {
      EnergyAwareSenderHelper energyAwareSenderHelper;
      double voltageTh = 0.3;
      double energyTh = (capacity/1000) * pow (voltageTh, 2) / 2; // capacity in mF
      energyAwareSenderHelper.SetEnergyThreshold (energyTh); // (0.29);
      energyAwareSenderHelper.SetMinInterval (Seconds(appPeriod));
      energyAwareSenderHelper.SetPacketSize (packetSize);
      energyAwareSenderHelper.Install (endDevices);
    }
  else
    {
      OneShotSenderHelper oneshot;
      oneshot.SetAttribute ("PacketSize", IntegerValue(packetSize));
      oneshot.Install (endDevices);

      // PeriodicSenderHelper periodicSenderHelper;
      // periodicSenderHelper.SetPeriod (Seconds (appPeriod));
      // periodicSenderHelper.SetPacketSize(packetSize);
      // periodicSenderHelper.Install (endDevices);
    }


  /************************
   * Install Energy Model *
   ************************/


  CapacitorEnergySourceHelper capacitorHelper;
  capacitorHelper.Set ("Capacity", DoubleValue (capacity/1000)); // capacity in mF
  capacitorHelper.Set ("CapacitorLowVoltageThreshold", DoubleValue (0.545454));
  capacitorHelper.Set ("CapacitorHighVoltageThreshold", DoubleValue (0.7));
  capacitorHelper.Set ("CapacitorMaxSupplyVoltageV", DoubleValue (3.3));
  capacitorHelper.Set ("CapacitorEnergySourceInitialVoltageV", DoubleValue (3.3));
  capacitorHelper.Set ("PeriodicVoltageUpdateInterval", TimeValue (Seconds (10)));

  LoraRadioEnergyModelHelper radioEnergy;
  radioEnergy.Set("EnterSleepIfDepleted", BooleanValue(false));
  radioEnergy.Set ("TurnOnDuration", TimeValue (Seconds(0.2)));
  // Values from datasheet 
  radioEnergy.Set ("TxCurrentA", DoubleValue (0.028)); // check - there are different values
  radioEnergy.Set ("IdleCurrentA", DoubleValue (0.0000015));
  radioEnergy.Set ("RxCurrentA", DoubleValue (0.011));
  radioEnergy.Set ("SleepCurrentA", DoubleValue (0.0000001));
  radioEnergy.Set ("StandbyCurrentA", DoubleValue (0.0000014));

  //  // Basic Energy harvesting
  BasicEnergyHarvesterHelper harvesterHelper;
  harvesterHelper.Set ("PeriodicHarvestedPowerUpdateInterval",
                       TimeValue (Seconds(10)));
  // // Visconti, Paolo, et al."An overview on state-of-art energy harvesting
  // //     techniques and choice criteria: a wsn node for goods transport and
  // //     storage powered by a smart solar-based eh system." Int.J .Renew.Energy Res
  // //     7(2017) : 1281 - 1295.
  // // Let's assume we are OUTDOOR, and the surface we are provided is 2 cm2
  // double minPowerDensity = 0.000001; // 30e-3;
  // double maxPowerDensity = 0.00001; // 0.30e-3;
  double minPowerDensity = eh; // 30e-3;
  double maxPowerDensity = eh; // 0.30e-3;

  std::string power = "ns3::UniformRandomVariable[Min=" + std::to_string(minPowerDensity) + "|Max=" + std::to_string(maxPowerDensity) + "]";
  harvesterHelper.Set ("HarvestablePower", StringValue (power));

  // // Variable energy harvesting
  VariableEnergyHarvesterHelper variableEhHelper;
  variableEhHelper.Set("Filename", StringValue(filenameHarvester));


  // install source on EDs' nodes
  EnergySourceContainer sources = capacitorHelper.Install (endDevices);
  // install device model
  DeviceEnergyModelContainer deviceModels =
    radioEnergy.Install (endDevicesNetDevices, sources);

  Names::Add ("/Names/EnergySource", sources.Get(0));

  // EnergyHarvesterContainer harvesters = harvesterHelper.Install (sources);
  if (enableVariableHarvester)
  {
    EnergyHarvesterContainer harvesters = variableEhHelper.Install (sources);
  }
  else
    {
      EnergyHarvesterContainer harvesters = harvesterHelper.Install (sources);
    }

  // Names::Add("Names/EnergyHarvester", harvesters.Get (0));
  // Ptr<EnergyHarvester> myHarvester = harvesters.Get(0);
  // myHarvester -> TraceConnectWithoutContext("TotalEnergyHarvested",
  //                                           MakeCallback(&OnEnergyHarvested));

  ////////////
  // Create NS
  ////////////

  NodeContainer networkServers;
  networkServers.Create (1);

  // Install the NetworkServer application on the network server
  NetworkServerHelper networkServerHelper;
  networkServerHelper.SetAttribute ("ReplyPayloadSize", IntegerValue(10));
  networkServerHelper.SetGateways (gateways);
  networkServerHelper.SetEndDevices (endDevices);
  networkServerHelper.Install (networkServers);

  // Install the Forwarder application on the gateways
  ForwarderHelper forwarderHelper;
  forwarderHelper.Install (gateways);

  ///////////////////////
  // Connect tracesources
  ///////////////////////

  // Names::Add("Names/myEDmac", endDevices.Get(0)->GetObject<EndDeviceLorawanMac>());
  Ptr<LoraNetDevice> loraDevice = gateways.Get (0)->GetDevice (0)->GetObject<LoraNetDevice> ();
  Ptr<LorawanMac> myGWmac = loraDevice->GetMac ()->GetObject<LorawanMac> ();

  myGWmac->TraceConnectWithoutContext ("ReceivedPacket",
                                       MakeCallback (&OnReceivedULPacketCallback));
  
  Ptr<LoraNetDevice> loraDeviceED = endDevices.Get (0)->GetDevice (0)->GetObject<LoraNetDevice> ();
  Ptr<LorawanMac> myEDmac = loraDeviceED->GetMac ()->GetObject<LorawanMac> ();

  myEDmac->TraceConnectWithoutContext ("ReceivedPacket",
                                       MakeCallback(&OnReceivedDLPacketCallback));
  myEDmac->TraceConnectWithoutContext ("CloseSecondReceiveWindow",
                                       MakeCallback (&OnCloseRX2Callback));

  /****************
  *  Simulation  *
  ****************/

  Simulator::Stop (Seconds (simTime));

  Simulator::Run ();

  if (!ULCycleCompleted)
    {
      std::cout << "0 0" << std::endl;
    }
  if (receivedULCallbackFirstCall)
    {
      std::cout << "1 0" << std::endl;
    }
  if (receivedDLCallbackFirstCall)
    {
      std::cout << "2 0" << std::endl;
    }

  Simulator::Destroy ();

  return 0;
}
