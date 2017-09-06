ESP8266 WeMos Shield for microchip LORAWAN RN2483 or RN2903
===========================================================

<img src="https://raw.githubusercontent.com/hallard/WeMos-RN2483/master/pictures/RN2483-Stacked.jpg" alt="WeMos With RN2483"> 

This shield is used to hold [RN2483][4] Software with WeMos ESP8266 boards it has just few minimal features. 
- I2C Pullups placement
- Classic I2C 128x64 OLED connector
- Placement for Microchip LoraWan [RN2483][4] or [RN2903][4] module
- Placement for choosing single Wire, SMA or u-FL Antenna type for both 433MHz and 868MHz output
- Solder pad choosing 2 RX/TX available on ESP8266 (GPIO1/GPIO3 or GPIO13/GPIO12 to avoid conflict with onboard USB/Serial of WeMos)
- WS2812B Type LED for visual indication
- Custom switch on GPIO14

You can find more information on WeMos on their [site][1], it's really well documented. There is also a nice blog post on Microchip module made by disk91 [here][5]
I now use WeMos boards instead of NodeMCU's one because they're just smaller, features remains the same, but I also suspect WeMos regulator far better quality than the one used on NodeMCU that are just fake of originals AMS117 3V3.

**V1.1 Boards arrived from OSHPark, I tested them, worked out of the box but I made new revision cand corrected some mistakes**
V1.2 contain the following changes (board only, nothing chnaged on schematic):
- Added isolate ground pane to avoid sometime short between header and ground after soldering headers 
- Changed Silk to Node instead of gateway 

**V1.0 Boards arrived from OSHPark, I tested them, worked out of the box but I made new revision cand corrected some mistakes**
V1.1 contain the following changes:
- Error with RX/TX swapped on ESP8266, they are GPIO15/GPIO13 nor GPIO12/GPIO13 (so with V1.0 you can use SoftSerial or Native RX/TX of ESP but not in swapped mode)
- I made larger pad for Microchip module, even if I have strong experience in soldering in SMD, it was a pain to do them see picture below, I suspected board/solder. Ok made some new ones, think now I've got the technique ;-)
- Switch moved from GPIO14 to GPIO2, so I removed pullup because WeMos already have one on GPIO2
- Removed GPIO0 and GPIO11 from RN2483 to avoid as much as possible closer PAD

**V1.1 Boards are tested and working, waiting to test V1.2 but should works, just minor changes**

Detailed Description
====================

Look at the schematics for more informations.

### Firmware

Firmware is currently under developement and may not work as you expected, but for now it works fine for our testing lab.
It's available under [firmware folder][9]. See dedicated [README.md][10].

### Schematic
![schematic](https://raw.githubusercontent.com/hallard/WeMos-RN2483/master/pictures/WeMos-RN2483-sch.png)  

### Boards 
<img src="https://raw.githubusercontent.com/hallard/WeMos-RN2483/master/pictures/WeMos-RN2483-top.png" alt="Top" width="40%" height="40%">&nbsp;
<img src="https://raw.githubusercontent.com/hallard/WeMos-RN2483/master/pictures/WeMos-RN2483-bot.png" alt="Bottom" width="40%" height="40%">&nbsp; 

You can order the PCB of this board at [OSHPARK][3] or at [PCBs.io][8]. But PCBs.io give me reward that allow me to buy some new created boards.

### Assembled boards (V1.0)

<img src="https://raw.githubusercontent.com/hallard/WeMos-RN2483/master/pictures/WeMos-RN2483-top.jpg" alt="Top">&nbsp;
<img src="https://raw.githubusercontent.com/hallard/WeMos-RN2483/master/pictures/WeMos-RN2483-bot.jpg" alt="Bottom">&nbsp; 
<img src="https://raw.githubusercontent.com/hallard/WeMos-RN2483/master/pictures/WeMos-RN2483-mod.jpg" alt="Bottom With RN2483">&nbsp; 

##License

<img alt="Creative Commons Attribution-NonCommercial 4.0" src="https://i.creativecommons.org/l/by-nc/4.0/88x31.png">   

This work is licensed under a [Creative Commons Attribution-NonCommercial 4.0 International License](http://creativecommons.org/licenses/by-nc/4.0/)    
If you want to do commercial stuff with this project, please contact [CH2i company](https://www.ch2i.eu/en#support) so we can organize an simple agreement.

##Misc
See news and other projects on my [blog][2] 
 
[1]: http://www.wemos.cc/Products/d1_mini.html
[2]: https://hallard.me
[3]: https://oshpark.com/shared_projects/G3MI8Wk2
[4]: http://www.microchip.com/wwwproducts/Devices.aspx?product=RN2483
[5]: https://www.disk91.com/2015/technology/networks/first-step-in-lora-land-microchip-rn2483-test/
[6]: https://github.com/hallard/WebSocketToSerial
[7]: http://www.microchip.com/wwwproducts/Devices.aspx?product=RN2903
[8]: https://PCBs.io/share/zMbEp
[9]: https://github.com/hallard/WeMos-RN2483-firmware/tree/master/WeMos-rn2483
[10]: https://github.com/hallard/WeMos-RN2483-firmware/blob/master/WeMos-rn2483/README.md
