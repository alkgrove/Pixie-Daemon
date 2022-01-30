###### Using THE Raspberry Pi as a nixie clock

I was given a set of nixie tubes long time ago as a gift and had a
design for a clock but never got to the point of laying it out. I found
on Etsy and ebay was that [GRA+AFCH](https://gra-afch.com/) selling
clock boards. This design is an add-on board to an Arduino which has
tube sockets, high voltage switcher, high voltage serial to parallel
ICs, and real time clock. The kicker being that the tubes are much
harder to get and very expensive. By exploring their web site I found
they had a Pi Shield, which does all of the necessary Pi to Arduino
level shifting. Unfortunately none of their cases will fit the raspberry
pi.

I wanted the Raspberry Pi because Linux makes a pretty good clock. it
supports NTP to set and keep the time accurate and a timezone database.
Once it is set up, I shouldn't have to set the time again and it should
keep good time. The Pi does has some disadvantages. It isn't well suited
to time critical IO but we have work arounds.

I picked the Raspberry Pi 3B+. It is less power than the Pi 4 and will
fit the mounting holes provided. It is running headless [Raspberry Pi OS
Lite](https://www.raspberrypi.com).

I got the GRA+AFCH Raspberry Pi code and it was written similar as their
Arduino code using WiringPi. Sadly WiringPi has been deprecated. I wrote
a daemon in C which can be run as a service and updates the Nixie Tube
clock and control the LEDs. In the future, I plan to add a presence
detector to turn the Nixie/LEDs on only when standing in front of the
clock.

I used a library [rpi-ws281x](https://github.com/jgarff/rpi_ws281x) to
control the LEDs. This gets around a lot of the timing issues of
modulating GPIO for the ws281x smart pixel LEDs. It uses a combination
of DMA and Timers. Unfortunately, the board isn't wired to accommodate
this so I needed to make a simple hardware change. I use a continuous
roll player piano or a simple music box model for this using json as the
roll. I use the [jsmn](https://github.com/zserge/jsmn) json parser to
convert the json into a C data structure that is cycled through to
control the LEDs.

The Nixie tubes have more forgiving timing and use the built in SPI and
memory mapped GPIO. The real time clock (RTC) on the GRA_AFCH board is a
DS3231 that the raspberry pi supports natively with some minor
modifications.

I wrote this the beginning of 2022. I'll provide links to stuff but your
best bet is to search, as with most links become broken in time.

##### Hardware Change

The LEDs do require a minor hardware modification to the top of the
raspberry pi adapter board. The rpi-ws281x uses a combination of DMA and
PWM timer to provide a jitter free data to the LEDs, unfortunate this
only can be done on a few pins which didn't map to the GRA-AFCH board.
The GRA-AFCH use a square pad for pin 1 reference consistently. Reading
the documentation for the rpi-ws281x, you will see that GPIO12 and
GPIO18 are only pins available for PWM0. I used GPIO18 or pin 12 of the
40 pin Pi connector X1 on the adapter board. This pin currently goes to
the Up button. You should see a trace from connector AN1 pin 3 to X1 pin
12, you should cut that trace. My suggestion for cutting the trace is to
use a sharp xacto knife and pull the knife through the trace with two
short parallel cuts about 0.2mm apart from each other perpendicular to
the trace. Cut this trace close to X1-12 pad but not that you're going
through solder. You need a fairly light touch and not gouge into the FR4
fiberglass while doing this. After several passes, you should have a
small rectangular chunk of copper flip come off the board and disappear
into the ether. Check the continuity between X1 pin 12 and AN1 pin 3, it
should now be open.

You should still have a little stub of trace sticking out of the AN1 pin
3 connector. You can scrape off the solder mask to expose the copper
going to AN1 pin 3. If you want to use the button, you can wire this to
an unused pin such as X1 pin 22 (GPIO25).

You will need to cut the trace going from connector X1 pin 36 to U2 pin
12. This trace goes under the chip but you want to cut it about 5mm from
the X1 pin 36 pad. Check to make sure there is no continuity between U2
pin 12 and X1 pin 36 pad. Next on the trace from the U2 pin 12 pad
carefully scrape away the soldermask and tin with solder. That is, flow
some solder onto this trace.

You then need to cut a wire that will go from connector X1 pin 12 to
this exposed trace. I solder the trace first and lay the wire flat on
the board. This is a fragile connection, so I suggest using either Q
dope or acrylic conformal coat to stress relief the wire from detaching
from this trace. Connect the other end to X1 pin 12.

![](media/image1.jpg){width="6.5in" height="3.859722222222222in"}You
could potentially work around cutting the traces but using the button or
enabling GPIO16 will make things not work well.

##### Software SETUP

As with any Raspberry Pi OS install, you will need a microSD card,
imager, and Raspberry Pi OS image. Just follow the instructions from the
site hosting the OS. Once the OS is imaged on the MicroSD, put the
MicroSD into a Windows box and open the boot directory. Create an empty
text file named ssh (no extension). I usually plug the Pi into Ethernet,
let it boot up and see what address the DHCP server assigned it from my
router. The Pi will need access to the internet for setting it up.

Once installed, you will need to enable SPI and I2C, they are disabled
by default. Start **sudo raspi-config** and using the arrow keys, select
**Interface Options** \<enter\> and then SPI and select Yes. Again
select **Interface Options** \<enter\> and then I2C and select Yes. If
you didn't set time zone or localization, they can be set from the
**Localisation Options** and is important to do so.

I use the rpi-ws281x library for controlling the LEDs. It uses a
combination of DMA and PWM to reliably control the serial stream to the
LEDs. The library needs to be downloaded and built on the pi.

Now install LED library and build using the following:

**sudo apt-get --y install gcc make cmake binutils git i2c-tools**

**cd \$HOME**

**git clone <https://github.com/jgarff/rpi_ws281x.git>**

**cd rpi-ws281x**

**mkdir build**

**cd build**

**cmake --D build_SHARED=OFF --D BUILD_TEST=ON ..**

**cmake --build .**

**sudo make install**

Next install the json parser and nixie daemon code and build:

**cd \$HOME**

**sudo git clone <https://github.com/zserge/jsmn.git>**

**sudo git clone <https://github.com/alkgrove/Pixie-Daemon.git>**

**cd Pixie-Daemon**

**sudo make**

**sudo make install**

**sudo systemctl daemon-reload**

##### Setting UP THE CLOCK CHIP

Connect the Pi, shield and Nixie board together and power up. Open up a
terminal to the pi and try

**i2cdetect -y 1**

There should be a single entry at 68, the rest are \--. This is the 7
bit hex slave address for the RTC and shows that the pi can see the
clock.

Adafruit has a good write up on real time clocks with the raspberry pi.
Most of the following is pulled from
https://learn.adafruit.com/adding-a-real-time-clock-to-raspberry-pi/set-rtc-time

edit /boot/config.txt with nano:

**sudo nano /boot/config.txt**

and add the line at the end of the file:

**dtoverlay=i2c-rtc,ds3231**

Now we need to get rid of the fake-hwclock with the following commands:

**sudo apt-get -y remove fake-hwclock**

**sudo update-rc.d -f fake-hwclock remove**

**sudo systemctl stop fake-hwclock**

**sudo systemctl disable fake-hwclock**

Next edit the file /lib/udev/hwclock-set and comment out the lines with
#\'s:

**if \[ -e /run/systemd/system \] ; then**

**exit 0**

**fi**

should be

**#if \[ -e /run/systemd/system \] ; then**

**\# exit 0**

**#fi**

and comment out the lines:

**/sbin/hwclock \--rtc=\$dev \--systz \--badyear**

and

**/sbin/hwclock \--rtc=\$dev --systz**

as:

**#/sbin/hwclock \--rtc=\$dev \--systz --badyear**

and

**#/sbin/hwclock \--rtc=\$dev --systz**

Reboot and test with date:

**date**

this should produce the right date and time. Next write this to the
hwclock with

**sudo hwclock -w**

**sudo hwclock -r**

I believe the script you changed does this automatically at some point,
this just checks that it works now.

You can further check things with:

**sudo timedatectl status**

It should show RTC time with date time which means it\'s working and it
will say n/a if still on fake-hwclock.

You can further check the NTP servers that are queried for the time with

**sudo timedatectl show-timesync**

The NTP server is what gets queried over the network to get and set the
correct Universal time. Normally these go to a pool of servers such as
debian.pool.org. Since my internal network is rather large and some
devices aren\'t allowed on the internet, I use one of my servers as an
NTP server and all devices go to it. It in turn, is the only one which
updates its clock to a remote NTP server. So I edit the
/etc/systemd/timesyncd.conf file and uncomment and change the line
NTP=\<myserver\>.

##### Configuring the LEDs

The LED action is controlled by a json structure and is modeled after a
music box or player piano roll. When the daemon starts, it will open the
file /usr/local/etc/LEDcolor.json. A default file is installed when
pixie-daemon is compiled and installed. There are sample files in the
Pixie-Daemon/assets folder.

Each file is made of two objects, "system" and "roll". System has
property "level" that will adjust the overall light level from 0 to 100
(percent). "roll" has the property value of an array of objects. These
objects have property "step" which can have a value "fast" or "slow",
"delay" which is an integer number of milliseconds and "color". Color
property value is an array of eight javascript-like RGB colors strings,
each string for an LED starting at the left for the first value. The
color representation is a string enclosed by quotes and starting with
'#' followed by three groups of two hexadecimal numbers each
representing 8-bit Red Green Blue Values.

If step is fast, the color will change immediately to the color array
and hold that color for delay number of milliseconds. If step is slow,
the color will change slowly from the current line to the next line and
the transition will take delay number of milliseconds. Delay values for
slow need to be in 25millisecond increments.

##### Starting the daemon

I would suggest starting the display using the command line especially
if trying a new LED color table. This is done with the command:

**/usr/local/bin/pixied**

Ctrl-c can be used to exit.

So the first time starting the daemon, let's set it up so it starts on
boot:

**sudo systemctl enabled pixied**

The nixie clock can be started with:

**sudo service pixied start**

If you need to stop the Nixie Clock use

**sudo service pixied stop**

You can see the status with

**sudo systemctl status pixied**
