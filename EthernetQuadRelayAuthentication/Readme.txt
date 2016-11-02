EthernetQuadRelayAuthentication

  Quad relay ethernet control with authentication
  
  was RudeBastardWebRelayControl
  http://www.rudebastard.com/article.php?story=20130310155336385
  "started this Arduino project with the objective to be able to login to an ethernet server on my home network and turn a computers power on and off. I have an old server that I seldom need and I don't want to leave running all of the time but when I do need it I want to be able to remotely power it up. I also run this website from a server on my network and it occasionally hangs and needs to be powered down and back up. This project, when complete will allow me to control up to four devices. I did my testing with a relay board but I will switch to solid state switches to do the actual power switching. Once I have completed the packaging I will post another article documenting what I did.
  Below is the Arduino/Webduino code. Webduino https://github.com/sirleech/Webduino
  I tested it in Chrome and it worked fine. When I tried it in IE9 I had an issue where IE set the Document Mode to quirks. When I set it to IE9 standards it worked fine.
  I was also able to test it on my Android phone using the web browser"

  Based on Web_Authentication.ino
  https://github.com/sirleech/Webduino/blob/master/examples/Web_Authentication/Web_Authentication.ino

  Added
  Serial debug but it eats memory!
  DHCP
  Shortened some text
  Pins are in define

  works with RelayShield by Catalex catalex.taobao.com V1.0 12/02/2013

  server is at port 80
  You have to connect to http://IPofArduino/control to get the control page
  Without /control you will get the EPIC FAIL page.
  This sketch uses DHCP so you need to set the router (the DHCP server) to
  assing the same IP to this Arduino.
  You can also assign the IP on the sketch but you have to code it.


  Connect to ArduinoIP:80/control
  admin:admin
  Change username and password as they are too much common
  Choose username and password, for example  dante and alighieri
  Go to https://www.base64encode.org/
  Insert dante:alighieri in the upper text box and leave UTF8
  Click on encode
  You will get un the bottom box a string like ZGFudGU6YWxpZ2hpZXJp (for dante:alighieri)
  Put int into CREDENTIALS define between the two "

  All serial instructions where added only for debug.
  They get a lot of memory so if you need memory
  remove them, for example to have longer strings.

  The initial relay state is defined in setup.
  It means that on reset the relays are resetted!
  If you need that they are not resetted you have to put
  their state on EEPROM and read again from there upon reset

  2016/10/21
  Changed the logic since in the original sketch was negate
  Now 0 mean relay off
