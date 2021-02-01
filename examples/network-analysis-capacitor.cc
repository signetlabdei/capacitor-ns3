/*
 * This script simulates a network to test capacitor source
 * and lora radio energy model.
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
#include "ns3/lora-packet-tracker.h"
#include "ns3/file-helper.h"
#include "ns3/names.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "src/core/model/config.h"
#include "src/lorawan/helper/variable-energy-harvester-helper.h"
#include "src/lorawan/model/capacitor-energy-source.h"
#include "src/lorawan/helper/capacitor-energy-source-helper.h"
#include "src/lorawan/model/lora-tx-current-model.h"
#include "ns3/correlated-shadowing-propagation-loss-model.h"
#include "ns3/building-penetration-loss.h"
#include "ns3/building-allocator.h"
#include "ns3/buildings-helper.h"
#include <algorithm>
#include <bits/stdint-uintn.h>
#include <cmath>
#include <ctime>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>
#include <sys/types.h>

using namespace ns3;
using namespace lorawan;

NS_LOG_COMPONENT_DEFINE ("NetworkAnalysisCapacitor");

// Inputs
double simTime = 3600;
double appPeriod = 300;
double capacitance= 200; // mF
int packetSize = 10;
uint nDevices = 1;
double radius = 1000;
double eh = 0.001;
double E = 3.3; // V
double voltageThLow = 0.545454; // 1.8 V
double voltageThHigh = 0.9090; // 3 V
double energyAwareSenderVoltageTh = 2.5; // V
uint8_t dr = 5;
int confirmed = 0;
bool realisticChannelModel = false; // Channel model
bool print = false; // Output control
std::string sender = "periodicSender";
std::string filenameRemainingVoltage = "remainingVoltage.txt";
std::string filenameEnergyConsumption = "energyConsumption.txt";
std::string filenameRemainingEnergy = "remainingEnergy.txt";
std::string filenameState = "deviceStates.txt";
std::string filenameTx = "deviceTx.txt";
std::string filenameEnoughEnergy = "energyEnoughForTx.txt";
std::string pathToInputFile = "/home/marty/work/ua/panels_data";
std::string filenameHarvesterSun = "/outputixys.csv";
std::string filenameHarvesterCloudy = "/outputixys_cloudy.csv";
bool remainingVoltageCallbackFirstCall = true;
bool energyConsumptionCallbackFirstCall = true;
bool enoughEnergyCallbackFirstCall = true;
bool stateChangeCallbackFirstCall = true;
bool TxCallbackFirstCall = true;
int generatedPacketsAPP = 0;

//////////////
// Callbcks //
//////////////

void
OnRemainingEnergyChange (double oldRemainingEnergy, double remainingEnergy)
{
  // NS_LOG_DEBUG (Simulator::Now().GetSeconds() << " " << remainingEnergy);
}

void
OnDeviceEnergyConsumption (double oldvalue, double energyConsumption)
{
  const char *c = filenameEnergyConsumption.c_str ();
  std::ofstream outputFile;
  if (energyConsumptionCallbackFirstCall)
    {
      // Delete contents of the file as it is opened
      outputFile.open (c, std::ofstream::out | std::ofstream::trunc);
      // Set the initial sleep state
      outputFile <<  "0 0" << std::endl;
      energyConsumptionCallbackFirstCall = false;
    }
  else
    {
    // Only append to the file
    outputFile.open (c, std::ofstream::out | std::ofstream::app);
    }

  outputFile << Simulator::Now ().GetSeconds () << " " << energyConsumption << std::endl;
  outputFile.close ();
}

void
OnRemainingVoltageChange (std::string context,
                          double oldRemainingVoltage, double remainingVoltage)
{

  // NS_LOG_DEBUG("Callback " << context);
  const char *c = filenameRemainingVoltage.c_str ();
  std::ofstream outputFile;
  if (remainingVoltageCallbackFirstCall)
    {
      // Delete contents of the file as it is opened
      outputFile.open (c, std::ofstream::out | std::ofstream::trunc);
      remainingVoltageCallbackFirstCall = false;
    }
  else
    {
      // Only append to the file
      outputFile.open (c, std::ofstream::out | std::ofstream::app);
    }

  outputFile << context << " "
             << Simulator::Now ().GetMilliSeconds ()
             << " " << remainingVoltage
             << std::endl;
  outputFile.close ();
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

void
CheckEnoughEnergyCallback (uint32_t nodeId, Ptr<const Packet> packet,
                           Time time, bool boolValue)
{
  const char *c = filenameEnoughEnergy.c_str ();
  std::ofstream outputFile;
  if (enoughEnergyCallbackFirstCall)
    {
      // Delete contents of the file as it is opened
      outputFile.open (c, std::ofstream::out | std::ofstream::trunc);
      enoughEnergyCallbackFirstCall = false;
    }
  else
    {
      // Only append to the file
      outputFile.open (c, std::ofstream::out | std::ofstream::app);
    }

  outputFile << Simulator::Now ().GetSeconds () << " " << boolValue << std::endl;
  outputFile.close ();

}


void
OnEndDeviceStateChange (std::string context,
                        EndDeviceLoraPhy::State oldstatus,
                        EndDeviceLoraPhy::State status)
{
  const char *c = filenameState.c_str ();
  std::ofstream outputFile;
  if (stateChangeCallbackFirstCall)
    {
      // Delete contents of the file as it is opened
      outputFile.open (c, std::ofstream::out | std::ofstream::trunc);
      // Set the initial sleep state
      outputFile << 0 << " " << 0 << std::endl;
      NS_LOG_DEBUG ("Append initial state inside the callback");
      stateChangeCallbackFirstCall = false;
    }
  else
    {
      // Only append to the file
      outputFile.open (c, std::ofstream::out | std::ofstream::app);
    }

  outputFile << context << " " <<Simulator::Now ().GetSeconds () << " " << status << std::endl;
  outputFile.close ();
}

void
OnEndDeviceTx (std::string context, EndDeviceLoraPhy::State oldstatus,
                        EndDeviceLoraPhy::State status)
{
  if (status == EndDeviceLoraPhy::TX)
    {
      NS_LOG_DEBUG("One transmission");
      const char *c = filenameTx.c_str ();
      std::ofstream outputFile;
      if (TxCallbackFirstCall)
        {
          outputFile.open (c, std::ofstream::out | std::ofstream::trunc);
          TxCallbackFirstCall = false;
        }
      else
        {
          // Only append to the file
          outputFile.open (c, std::ofstream::out | std::ofstream::app);
        }
      outputFile << context << " " << Simulator::Now ().GetSeconds () << std::endl;
      outputFile.close ();
    }
}

  void OnGeneratedPacket (void)
  {
    // NS_LOG_DEBUG("Generated packet at APP level");
    // In the whole network
    generatedPacketsAPP = generatedPacketsAPP + 1;
    // NS_LOG_DEBUG ("Total number of generated APP packet = " << generatedPacketsAPP);
  }

  /***********
 ** MAIN  **
 **********/
  int main (int argc, char *argv[])
  {

    // Inputs
    CommandLine cmd;
    cmd.AddValue ("pathToInputFile",
                  "Absolute path till the input file for the varaible energy harvester",
                  pathToInputFile);
    cmd.AddValue ("nDevices", "Number of nodes", nDevices);
    cmd.AddValue ("capacitance", "capacitance[mF]", capacitance);
    cmd.AddValue ("simTime", "Simulation time [s]", simTime);
    cmd.AddValue ("appPeriod", "App period [s]", appPeriod);
    cmd.AddValue ("confirmed", "Percentage of devices sending confirmed message", confirmed);
    cmd.AddValue ("dr", "dr (dr=-1 = based on the channel)", dr);
    cmd.AddValue ("packetSize", "PacketSize", packetSize);
    // cmd.AddValue ("enableVariableHarvester", "Enable harvester from input file",
    //                enableVariableHarvester);
    // cmd.AddValue ("sun", "Input from sunny day", sun);
    cmd.AddValue ("eh", "Harvested power [W] - eh=-1 data for sunny day, eh=-2 data for cloudy day",
                  eh);
    cmd.AddValue ("sender", "Application sender [energyAwareSender, periodicSender, multipleShots]",
                  sender);
    cmd.AddValue ("energyAwareSenderVoltageTh", "Voltage threshold above which the packet is sent",
                  energyAwareSenderVoltageTh);
    cmd.Parse (argc, argv);

    // Set up logging
    LogComponentEnable ("NetworkAnalysisCapacitor", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraPacketTracker", LOG_LEVEL_ALL);
    // LogComponentEnable ("CapacitorEnergySource", LOG_LEVEL_ALL);
    // LogComponentEnable ("CapacitorEnergySourceHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraRadioEnergyModel", LOG_LEVEL_ALL);
    LogComponentEnable ("EnergyAwareSender", LOG_LEVEL_ALL);
    LogComponentEnable ("EnergyAwareSenderHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("EnergyHarvester", LOG_LEVEL_ALL);
    // LogComponentEnable ("VariableEnergyHarvester", LOG_LEVEL_ALL);
    // LogComponentEnable ("EnergySource", LOG_LEVEL_ALL);
    // LogComponentEnable ("BasicEnergySource", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraChannel", LOG_LEVEL_INFO);
    // LogComponentEnable ("LoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable ("EndDeviceLoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable ("SimpleEndDeviceLoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable ("GatewayLoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable ("SimpleGatewayLoraPhy", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraInterferenceHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("LorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable ("EndDeviceLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable ("ClassAEndDeviceLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable ("GatewayLorawanMac", LOG_LEVEL_ALL);
    // LogComponentEnable ("LogicalLoraChannelHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("LogicalLoraChannel", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraPhyHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("LorawanMacHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("OneShotSenderHelper", LOG_LEVEL_ALL);
    // LogComponentEnable ("OneShotSender", LOG_LEVEL_ALL);
    // LogComponentEnable ("PeriodicSender", LOG_LEVEL_ALL);
    // LogComponentEnable ("LorawanMacHeader", LOG_LEVEL_ALL);
    // LogComponentEnable ("LoraFrameHeader", LOG_LEVEL_ALL);

    LogComponentEnableAll (LOG_PREFIX_FUNC);
    LogComponentEnableAll (LOG_PREFIX_NODE);
    LogComponentEnableAll (LOG_PREFIX_TIME);

    // Set SF
    Config::SetDefault ("ns3::EndDeviceLorawanMac::DataRate", UintegerValue (dr));
    // Set MAC behavior
    Config::SetDefault ("ns3::EndDeviceLorawanMac::MacTxIfEnergyOk", BooleanValue (false));

    // Select harvester and input file
    bool enableVariableHarvester;
    std::string filenameHarvester;
    if (eh == -1) // sunny
      {
        enableVariableHarvester = true;
        filenameHarvester = pathToInputFile + filenameHarvesterSun;
      }
    else if (eh == -2) // cloudy
      {
        enableVariableHarvester = true;
        filenameHarvester = pathToInputFile + filenameHarvesterCloudy;
      }
    else // constant harvester
      {
        enableVariableHarvester = false;
      }

    /************************
  *  Create the channel  *
  ************************/

    // NS_LOG_INFO ("Creating the channel...");

    // Create the lora channel object
    Ptr<LogDistancePropagationLossModel> loss = CreateObject<LogDistancePropagationLossModel> ();
    loss->SetPathLossExponent (3.76);
    loss->SetReference (1, 7.7);

    if (realisticChannelModel)
      {
        // Create the correlated shadowing component
        Ptr<CorrelatedShadowingPropagationLossModel> shadowing =
            CreateObject<CorrelatedShadowingPropagationLossModel> ();

        // Aggregate shadowing to the logdistance loss
        loss->SetNext (shadowing);

        // Add the effect to the channel propagation loss
        Ptr<BuildingPenetrationLoss> buildingLoss = CreateObject<BuildingPenetrationLoss> ();

        shadowing->SetNext (buildingLoss);
      }

    Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel> ();

    Ptr<LoraChannel> channel = CreateObject<LoraChannel> (loss, delay);

    /************************
  *  Create the helpers  *
  ************************/

    // NS_LOG_INFO ("Setting up helpers...");

    // Create the LoraPhyHelper
    LoraPhyHelper phyHelper = LoraPhyHelper ();
    phyHelper.SetChannel (channel);

    // Create the LorawanMacHelper
    LorawanMacHelper macHelper = LorawanMacHelper ();

    // Create the LoraHelper
    LoraHelper helper = LoraHelper ();
    helper.EnablePacketTracking ();

    /************************
  *  Create End Devices  *
  ************************/

    // NS_LOG_INFO ("Creating the end device...");

    // Create a set of nodes
    NodeContainer endDevices;
    endDevices.Create (nDevices);

    // ED's mobility
    MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::UniformDiscPositionAllocator", "rho", DoubleValue (radius),
                                   "X", DoubleValue (0.0), "Y", DoubleValue (0.0));
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    // Assign a mobility model to the node
    mobility.Install (endDevices);

    // Make it so that nodes are at a certain height > 0
    for (NodeContainer::Iterator j = endDevices.Begin (); j != endDevices.End (); ++j)
      {
        Ptr<MobilityModel> mobility = (*j)->GetObject<MobilityModel> ();
        Vector position = mobility->GetPosition ();
        position.z = 1.2;
        mobility->SetPosition (position);
      }

    // Create the LoraNetDevices of the end devices
    uint8_t nwkId = 54;
    uint32_t nwkAddr = 1864;
    Ptr<LoraDeviceAddressGenerator> addrGen =
        CreateObject<LoraDeviceAddressGenerator> (nwkId, nwkAddr);

    // Create the LoraNetDevices of the end devices
    macHelper.SetAddressGenerator (addrGen);
    // Create the LoraNetDevices of the end devices
    phyHelper.SetDeviceType (LoraPhyHelper::ED);
    macHelper.SetDeviceType (LorawanMacHelper::ED_A);
    NetDeviceContainer endDevicesNetDevices = helper.Install (phyHelper, macHelper, endDevices);

    // Set message type (Default is unconfirmed)
    if (confirmed > 0)
      {
        for (int i = 0; i == std::floor (nDevices * confirmed / 100); ++i)
          {
            Ptr<LorawanMac> edMac =
                endDevices.Get (i)->GetDevice (0)->GetObject<LoraNetDevice> ()->GetMac ();
            Ptr<ClassAEndDeviceLorawanMac> edLorawanMac =
                edMac->GetObject<ClassAEndDeviceLorawanMac> ();
            edLorawanMac->SetMType (LorawanMacHeader::CONFIRMED_DATA_UP);
          }
      }

    /*********************
   *  Create Gateways  *
   *********************/
    // NS_LOG_INFO ("Creating the gateway...");
    NodeContainer gateways;
    gateways.Create (1);

    Ptr<ListPositionAllocator> allocator = CreateObject<ListPositionAllocator> ();
    allocator->Add (Vector (0, 0, 0));
    mobility.SetPositionAllocator (allocator);
    mobility.Install (gateways);

    // Create a netdevice for each gateway
    phyHelper.SetDeviceType (LoraPhyHelper::GW);
    macHelper.SetDeviceType (LorawanMacHelper::GW);
    helper.Install (phyHelper, macHelper, gateways);

    if (dr < 0)
      {
        macHelper.SetSpreadingFactorsUp (endDevices, gateways, channel);
      }

    /**********************
   *  Handle buildings  *
   **********************/

    double xLength = 130;
    double deltaX = 32;
    double yLength = 64;
    double deltaY = 17;
    int gridWidth = 2 * radius / (xLength + deltaX);
    int gridHeight = 2 * radius / (yLength + deltaY);
    if (realisticChannelModel == false)
      {
        gridWidth = 0;
        gridHeight = 0;
      }
    Ptr<GridBuildingAllocator> gridBuildingAllocator;
    gridBuildingAllocator = CreateObject<GridBuildingAllocator> ();
    gridBuildingAllocator->SetAttribute ("GridWidth", UintegerValue (gridWidth));
    gridBuildingAllocator->SetAttribute ("LengthX", DoubleValue (xLength));
    gridBuildingAllocator->SetAttribute ("LengthY", DoubleValue (yLength));
    gridBuildingAllocator->SetAttribute ("DeltaX", DoubleValue (deltaX));
    gridBuildingAllocator->SetAttribute ("DeltaY", DoubleValue (deltaY));
    gridBuildingAllocator->SetAttribute ("Height", DoubleValue (6));
    gridBuildingAllocator->SetBuildingAttribute ("NRoomsX", UintegerValue (2));
    gridBuildingAllocator->SetBuildingAttribute ("NRoomsY", UintegerValue (4));
    gridBuildingAllocator->SetBuildingAttribute ("NFloors", UintegerValue (2));
    gridBuildingAllocator->SetAttribute (
        "MinX", DoubleValue (-gridWidth * (xLength + deltaX) / 2 + deltaX / 2));
    gridBuildingAllocator->SetAttribute (
        "MinY", DoubleValue (-gridHeight * (yLength + deltaY) / 2 + deltaY / 2));
    BuildingContainer bContainer = gridBuildingAllocator->Create (gridWidth * gridHeight);

    BuildingsHelper::Install (endDevices);
    BuildingsHelper::Install (gateways);

    /*********************************************
   *  Install applications on the end devices  *
   *********************************************/

    if (sender == "energyAwareSender")
      {
        EnergyAwareSenderHelper energyAwareSenderHelper;
        double energyTh = capacitance / 1000 * pow (energyAwareSenderVoltageTh, 2) / 2;
        NS_LOG_WARN (voltageThHigh * E);
        energyAwareSenderHelper.SetEnergyThreshold (energyTh);
        energyAwareSenderHelper.SetMinInterval (Seconds (appPeriod));
        energyAwareSenderHelper.SetPacketSize (packetSize);
        energyAwareSenderHelper.Install (endDevices);
      }
    else if (sender == "multipleShots")
      {
        OneShotSenderHelper oneshotsenderHelper;
        double sendTime = appPeriod;
        while (sendTime < simTime)
          {
            oneshotsenderHelper.SetSendTime (Seconds (sendTime));
            oneshotsenderHelper.Install (endDevices);
            oneshotsenderHelper.SetAttribute ("PacketSize", IntegerValue (packetSize));
            sendTime = sendTime + appPeriod;
          }
      }
    else if (sender == "periodicSender")
      {
        PeriodicSenderHelper periodicSenderHelper;
        periodicSenderHelper.SetPeriod (Seconds (appPeriod));
        periodicSenderHelper.SetPacketSize (packetSize);
        periodicSenderHelper.Install (endDevices);
      }

    /************************
   * Install Energy Model *
   ************************/
    std::string rv = "ns3::UniformRandomVariable[Min=0|Max=" + std::to_string (E) + "]";
    Config::SetDefault ("ns3::CapacitorEnergySource::RandomInitialVoltage", StringValue (rv));

    CapacitorEnergySourceHelper capacitorHelper;
    capacitorHelper.Set ("Capacitance", DoubleValue (capacitance / 1000));
    capacitorHelper.Set ("CapacitorLowVoltageThreshold", DoubleValue (voltageThLow));
    capacitorHelper.Set ("CapacitorHighVoltageThreshold", DoubleValue (voltageThHigh));
    capacitorHelper.Set ("CapacitorMaxSupplyVoltageV", DoubleValue (3.3));
    // Assumption that the C does not reach full capacitance because of some
    // consumption in the OFF state
    // double Ioff = 0.0000055;
    // double RLoff = E/Ioff;
    // double ri = pow(E, 2)/eh;
    // double Req_off = RLoff;
    // double V0 = E;
    // if (eh != 0)
    // {
    //   Req_off = RLoff * ri / (RLoff + ri);
    //   V0 = E * Req_off / ri;
    //   }
    // NS_LOG_DEBUG ("Initial voltage [V]= " << V0 <<
    //               " RLoff " << RLoff <<
    //               " ri " << ri <<
    //               " R_eq_off " << Req_off);
    // Set initial voltage uniformy random in (0, E)
    capacitorHelper.Set ("RandomInitialVoltage", StringValue (rv));

    capacitorHelper.Set ("PeriodicVoltageUpdateInterval", TimeValue (MilliSeconds (600)));
    // capacitorHelper.Set ("FilenameVoltageTracking", StringValue (filenameRemainingVoltage));

    //  // Basic Energy harvesting
    BasicEnergyHarvesterHelper harvesterHelper;
    harvesterHelper.Set ("PeriodicHarvestedPowerUpdateInterval", TimeValue (Seconds (10)));
    // // Constant harvesting rate
    double minPowerDensity = eh; // 30e-3;
    double maxPowerDensity = eh; // 0.30e-3;
    std::string power = "ns3::UniformRandomVariable[Min=" + std::to_string (minPowerDensity) +
                        "|Max=" + std::to_string (maxPowerDensity) + "]";
    harvesterHelper.Set ("HarvestablePower", StringValue (power));

    // // Variable energy harvesting
    VariableEnergyHarvesterHelper variableEhHelper;
    variableEhHelper.Set ("Filename", StringValue (filenameHarvester));

    LoraRadioEnergyModelHelper radioEnergy;
    radioEnergy.Set ("EnterSleepIfDepleted", BooleanValue (false));
    radioEnergy.Set ("TurnOnDuration", TimeValue (Seconds (0.3)));
    radioEnergy.Set ("TurnOnCurrentA", DoubleValue (0.015));
    radioEnergy.Set ("TxCurrentA", DoubleValue (0.028011));
    radioEnergy.Set ("IdleCurrentA", DoubleValue (0.000007));
    radioEnergy.Set ("RxCurrentA", DoubleValue (0.011011));
    radioEnergy.Set ("SleepCurrentA", DoubleValue (0.0000056));
    radioEnergy.Set ("StandbyCurrentA", DoubleValue (0.010511));
    radioEnergy.Set ("OffCurrentA", DoubleValue (0.0000055));

    // INSTALLATION ON EDs

    // install source on EDs' nodes
    EnergySourceContainer sources = capacitorHelper.Install (endDevices);
    // install device model
    DeviceEnergyModelContainer deviceModels = radioEnergy.Install (endDevicesNetDevices, sources);

    // Names::Add ("/Names/EnergySource", sources.Get(0));

    if (enableVariableHarvester)
      {
        EnergyHarvesterContainer harvesters = variableEhHelper.Install (sources);
      }
    else
      {
        EnergyHarvesterContainer harvesters = harvesterHelper.Install (sources);
      }

    ///////////////////////
    // Connect tracesources
    ///////////////////////
    for (uint j = 0; j < nDevices; ++j)
      {
        Ptr<Node> node = endDevices.Get (j);
        Names::Add ("Names/nodeApp"+std::to_string(j), node->GetApplication (0));
        Ptr<LoraNetDevice> loraNetDevice = node->GetDevice (0)->GetObject<LoraNetDevice> ();
        Ptr<EnergySource> capacitorES;
        for (uint k = 0; k < nDevices; ++k)
        {
          uint32_t nodeId = sources.Get (k)->GetNode ()->GetId ();
          if (nodeId == j)
            {
              capacitorES = sources.Get(k);
            }
          }
        Ptr<EndDeviceLoraPhy> phy = loraNetDevice->GetPhy ()->GetObject<EndDeviceLoraPhy> ();
        Ptr<EndDeviceLorawanMac> mac = loraNetDevice->GetMac ()->GetObject<EndDeviceLorawanMac> ();
        // mac->TraceConnectWithoutContext ("EnoughEnergyToTx",
        //                                      MakeCallback (&CheckEnoughEnergyCallback));
        // std::string name = "Names/nodeEdLoraPhy/"
        //   << std::to_string(endDevices.Get(j)->GetId()) << "/";

        phy->TraceConnect ("EndDeviceState", std::to_string (j),
                           MakeCallback (&OnEndDeviceStateChange));
        phy->TraceConnect ("EndDeviceState", std::to_string (j),
                           MakeCallback (&OnEndDeviceTx));
        capacitorES -> TraceConnect("RemainingVoltage", std::to_string(j),
                                    MakeCallback (&OnRemainingVoltageChange));

        ns3::Config::ConnectWithoutContext ("/Names/nodeApp"+std::to_string(j)+"/GeneratedPacket",
                                            MakeCallback (&OnGeneratedPacket));
        // NS_LOG_DEBUG ("Tracesources connected for ED " << j);
      }

    ////////////
    // Create NS
    ////////////

    NodeContainer networkServers;
    networkServers.Create (1);

    // Install the NetworkServer application on the network server
    NetworkServerHelper networkServerHelper;
    networkServerHelper.SetGateways (gateways);
    networkServerHelper.SetEndDevices (endDevices);
    networkServerHelper.Install (networkServers);

    // Install the Forwarder application on the gateways
    ForwarderHelper forwarderHelper;
    forwarderHelper.Install (gateways);

    ///////////////////
    // Packet tracker
    ///////////////////
    LoraPacketTracker &tracker = helper.GetPacketTracker ();

    /****************
  *  Simulation  *
  ****************/

    Simulator::Stop (Seconds (simTime));

    Simulator::Run ();

    //////////
    // Outputs
    //////////
    std::string pdr = tracker.CountMacPacketsGlobally (Seconds (0), Seconds (simTime));
    std::string cpsr = "0 0";
    if (confirmed > 0)
      {
        cpsr = tracker.CountMacPacketsGloballyCpsr (Seconds (0), Seconds (simTime));
      }
    // if (nDevices == 1)
    //   {
    //     std::cout << generatedPacketsAPP << " " << pdr << " " << cpsr << std::endl;
    //     std::vector<uint> v = tracker.CountMacPacketsPerEd(Seconds(0), Seconds(simTime), 0);
    //     std::cout << std::to_string(v.at(0)) << " " << std::to_string(v.at(1)) << std::endl;
    //   }

    std::cout << pdr << " " << cpsr << " ";
    std::cout << tracker.PrintPhyPacketsPerGw (Seconds (0), Seconds (simTime),
                                               gateways.Get (0)->GetId ())
              << std::endl;

    if (print && nDevices == 1)
      {
        // Avoid non-existent files
        NS_LOG_DEBUG ("Create file if not done yet: " << stateChangeCallbackFirstCall);
        // Fix output files if not created
        if (stateChangeCallbackFirstCall)
          {
            const char *c = filenameState.c_str ();
            std::ofstream outputFile;
            // Delete contents of the file as it is opened
            outputFile.open (c, std::ofstream::out | std::ofstream::trunc);
            // Set the initial sleep state
            outputFile << 0 << " " << 0 << std::endl;
            NS_LOG_DEBUG ("Append initial state");
            stateChangeCallbackFirstCall = false;
            outputFile.close ();
          }

        if (energyConsumptionCallbackFirstCall)
          {
            const char *c = filenameEnergyConsumption.c_str ();
            std::ofstream outputFile;
            // Delete contents of the file as it is opened
            outputFile.open (c, std::ofstream::out | std::ofstream::trunc);
            // Set the initial sleep state
            outputFile << 0 << " " << 0 << std::endl;
            NS_LOG_DEBUG ("Append initially not enough energy, because never called");
            energyConsumptionCallbackFirstCall = false;
            outputFile.close ();
          }

        if (enoughEnergyCallbackFirstCall)
          {
            const char *c = filenameEnoughEnergy.c_str ();
            std::ofstream outputFile;
            // Delete contents of the file as it is opened
            outputFile.open (c, std::ofstream::out | std::ofstream::trunc);
            // Set the initial
            outputFile << 0 << " " << 0 << std::endl;
            NS_LOG_DEBUG ("Append initially not enough energy, because never called");
            enoughEnergyCallbackFirstCall = false;
            outputFile.close ();
          }
      }

    // Destroy simulator
    Simulator::Destroy ();

    return 0;
  }
