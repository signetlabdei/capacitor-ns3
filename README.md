# Battery-less Capacitor-based IoT nodes in ns-3 #


This is an [ns-3](https://www.nsnam.org "ns-3 Website") code including
- the implementation of battery-less capacitor-based IoT nodes
- the implementation of an Energy-Harvesting source taking as input traces provided by the user
- an updated version of the [`lorawan` ns-3 module](https://github.com/signetlabdei/lorawan) employed to obtain simulations of a LoRaWAN network with battery-less capacitor-based end devices.

In the current version, original contribution include:
- Capacitor Energy Source 
- Variable Energy Harvester 
- Integration of the new components in the existing `lorawan` module


To avoid compilation errors, in `ns3/src/energy/model/energy-source.h`, the _private_ variables should be moved to _protected_.



## Main script ##

- `examples/energy-single-device-example`



## Reference ##
- M. Capuzzo, C. Delgado, J. Famaey, A. Zanella, "A ns-3 implementation of a battery-less nodefor energy-harvesting Internet of Things", WNS3 2021, June 23–24, 2021, Virtual Event, USA.



## Additional references ##

- [ns-3 tutorial](https://www.nsnam.org/docs/tutorial/html "ns-3 Tutorial")
- [ns-3 manual](https://www.nsnam.org/docs/manual/html "ns-3 Manual")
- The LoRaWAN specification can be requested at the [LoRa Alliance
  website](http://www.lora-alliance.org)
