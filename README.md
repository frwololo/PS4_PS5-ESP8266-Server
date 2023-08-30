# PS4_PS5-ESP8266-Server
A WebServer+Wifi Repeater+Fake DNS Server to Host PS4/PS5 Exploits on ESP8266

![exploit_in_action](https://github.com/frwololo/PS4_PS5-ESP8266-Server/assets/2976011/51168267-10d6-4247-ad61-b3bfecf821da)


## Details and usage
This is an implementation of a barebones webServer to Host PS4/PS5 Exploits on an ESP86, that also acts as a Wifi Repeater to maintain internet access for the console. This tool has the following features:
- Basic Webserver to host PS4 and PS5 exploits (including limited support for HTTPS to handle the PS5)
- FakeDNS that redirects playstation.net (user guides) to the web server, and blocks other playstation addresses
- The Access Point acts as a Wifi Repeater so that the clients (PS4, PS5, PC) can still access the internet for domains that are not blocked/redirected by the DNS
- Rudimentary config page to set up your Wifi credentials
- By default this ships with GoldHEN 2.3 for PS4 5.05, and the PS5 3.xx-4.xx Exploit. Those things can be replaced with other webkit-based exploits (e.g. for other firmwares) by modifying the contents of the Data folder. (flashing the ESP8266 firmware will be necessary to update those files)
- Also works in "offline mode", similar to other PS4 ESP8266 hosts out there, if wifi is disabled in config (default).

In other words, it's a self contained solution to run the PS4/PS5 exploits while still keeping Internet access

### Limitations
- Generally speaking **this shouldn't be considered "production ready" and you're using this at your own risk**. I do not guarantee any level of support. But just in case if you have any questions you can reach out at https://twitter.com/frwololo and https://wololo.net
- In my tests, Internet deconnections and/or random issues and crashes are frequent. This remains good enough to run the exploits and do a bit of browsing though.
- Because of heap memory limitation (both the HTTPS WebServer and the NAT routing use a lot of it) I wasn't able to enable all "Web Admin" features that can be found on Stooged's servers for file management and the like. In particular, it is not possible to update/remove the exploit files with the Web Interface currently. Flashing the device is required.

### How to Use
#### Initial Install and Setup
- Flash a precompiled binary (from [release page](https://github.com/frwololo/PS4_PS5-ESP8266-Server/releases)) to your ESP8266, for example using [NodeMCU Flasher](https://github.com/marcelstoer/nodemcu-pyflasher) (Select Yes to Erase Flash)
- After flashing, reset the device (e.g. by unplugging then re-plugging it from USB)
- Connect to Wifi access point "PS5_WEB_AP" with your computer (password is "password"). Notice that this can be very slow to connect, because the device can't access the internet yet, and Windows isn't happy about that.
- ![wifi_connection](https://github.com/frwololo/PS4_PS5-ESP8266-Server/assets/2976011/1b8eed2d-929f-4689-85c2-3cc5ef97c45e)

- Once connected to the Access point, go to 10.1.1.1/admin.html in your PC's web browser
- ![config_init](https://github.com/frwololo/PS4_PS5-ESP8266-Server/assets/2976011/b2ef5734-4dbf-4e97-9445-c8e205cd32aa)

- You should see the config settings, where you can (should) set ENABLE WIFI to 1, and below that enter the SSID and Password for your Home's Wifi Router
  - ![config_ok](https://github.com/frwololo/PS4_PS5-ESP8266-Server/assets/2976011/989e4059-4ee7-4df5-8cbc-5bcdb4d76ab4)

  - This will set the ESP8266 into the self contained mode with Fake DNS, Internet Access, etc...
- Once config is saved, the ESP8266 should restart on its own (if it doesn't, unplug then replug it)
- You're all set. At this point you probably have to connect your PC again to the access point for good measure, since it has changed.
- Things you should try from your PC Browser to confirm everything's working as expected (make sure you're only connected to the ESP8266 Access Point, disconnect your LAN cable if you have one):
  - http://10.1.1.1 should display the exploit page
  - https://10.1.1.1 should give you a certificate warning (proceed anyway of course), then redirect you to http://10.1.1.1 and you should ses the exploit
  - https://playstation.net should also send you to the exploit (after a certificate warning)
  - http://playstation.com should be blocked
  - Other websites (e.g. google.com) should be accessible

#### How to use On PS4/PS5
- Set up your console's Network Settings to connect via Wifi, using your ESP8266 Access point ("PS5_WEB_AP" by default, and password "password")
- On PS4 or PS5, go to Settings > User's Guide > User's Guide, this should load the exploit page.

There are a bunch of tutorials on how to run these out there, once ths host is set up it is no different from others, except for the fact that it allows the console to access internet

#### Notes
- If you want/need to get everything back offline like "other" ESP8266 Hosts, go back to 10.1.1.1/admin.html, set "ENABLE WIFI" to 0, then save the config again.
- To use different exploits (new version of GoldHEN, exploits for different firmwares, etc...), you currently need to upload the files directly to the ESP8266's flash, either by using Sketch Data Upload on Arduino IDE, or any other way that works for you. Can't implement WebAdmin for that due to RAM limitation. Happy if someone knows how to solve the issue

## How to Build
- Load the .ino file in Arduino IDE
- Dependencies: https://github.com/esp8266/Arduino (esp8266 Library, accessible through Arduino IDE directly), and https://github.com/esp8266/arduino-esp8266fs-plugin (Sketch Data upload)
- When building for your ESP8266 device, make sure that the following settings are correct:
  - Debug port/Debug Level: Disabled and None respectively (or equivalent). Debug functions take some space in RAM and we need everything we can get
  - lwip variant: "v2 lower memory works" for me. Other might be fine, but don't select one that says "no features" (we use the lwip NAT features in this project)
  - Flash Size: 4MB (with 3MB for FileSystem <-- need quite some space to store the exploits)
 
  ![nodemcu_settings](https://github.com/frwololo/PS4_PS5-ESP8266-Server/assets/2976011/487755d7-5f3b-437d-b27d-44f4e9038ecd)


## FAQ and Troubleshooting
### How good is the FakeDNS?
It's a very rough design that just looks for some specific domain names (currently hardcoded inside the main source file) and either redirect those to the local ESP WebServer (namely playstation.net, where the user guides are hosted), or blocks them (other kwnonw PlayStation telemetry domains). Just because the DNS suggests that these domains should be redirect or blocked, doesn't mean the client device can't do whatever they like. In the case of the PS4 and PS5, this seems to be enough, but I can't guarantee that the console isn't bypassing DNS replies, and (for example) asks another DNS on the Network. I can imagine this would be technically doable now that the ESP86 opens Internet access.

Furthermore, there's no support for regexps at the moment so it's really a simple string check in its current state.

### Issues accessing ESP8266 HTTPS Server
If you get some "Error Connection close" when testing https://10.1.1.1 (local HTTPS Server), it is possible the device is running out of Heap memory. For a "regular" user, just try to reset the device. For developers, try to reduce the value of NAPT in the main .ino file.
  
## Technical thoughts and stuff
### Why
Multiple versions of the ESP8266 Hosts exist to host PS4/PS5 exploits, for the most part based on work by Stooged (https://github.com/stooged/). To my knowledge however, none of them allow the clients (PS4, PS5, or the PC that you inevitably want to connect to them) to access Internet. The Access Point is generally stuck as a "Local Network" provider. This is enough to provide basic exploit access to the console, but there might be cases where we want to maintain Internet access anyway.

Most people achieve that by using a "Fake DNS" and either hosting the exploit locally on their PC ( https://wololo.net/2022/10/04/tutorial-running-the-ps5-4-03-exploit-on-windows-with-additional-dns-security-telemetry-blocking-etc/ ) or accessing one of the many "exploit hosts" online.

Given that the ESP8266 is able to simultaneously act as an an Access Point AND connect to a Wifi Router, I assumed there had to be ways it could act as a self contained Web Server + Fake DNS + Wifi Repeater, to mimic the other solutions. Turns out it is possible, with some limitations.

### Technical considerations
There are samples showing us how to run an HTTPS WebServer on ESP8266, how to block specific domain names with some ad-blocking DNS, how to enable NAT to use th device as a Wifi Repeater. There wasn't any example of how these things are all put together, so I guess this is now it.
Technically speaking, putting all these components together isn't particularly hard: The HTTP and HTTPS WebServer, including their content (exploits-related redirections, webAdmin) were takend from projects by [Stooged](https://github.com/stooged), which are widely used in the PS4/PS5 scene on multiple variations of the ESP8266 Hosts. The Default DNS Server however, is designed in a way that it will redirect all traffic to the AP Host (or that's how I understood it at least), so I replaced it with a modified version by [Rubfi](https://github.com/rubfi) which did more or less what I wanted. Last but not least, "Wifi Repeater" samples were available (technically, NAT routing) e.g. at https://github.com/AliBigdeli/Arduino-ESP8266-Repeater. 

## Credits
Code was scavenged from the following sources to build this thing:
- https://github.com/stooged/PS4-Server (general PS4 Exploit hosting, WebAdmin code, webServer and Wifi AP Code)
- https://github.com/stooged/PS5-Server (https Server code and PS5 exploit hosting)
- https://github.com/stooged/bin2html (Tool to convert .bin payloads into javascript to be directly inserted as a self contained PS4 5.05 exploit) 
- https://github.com/rubfi/esphole/tree/master (ad blocker DNS Server for ESP8266)
- https://github.com/AliBigdeli/Arduino-ESP8266-Repeater (NAT for ESP8266 sample)
