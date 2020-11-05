# QTube - Enqueue YouTube audio to a MPD server playlist # 

Since RaspiCast stopped working for me, possibly because of some server-side changes, I coded this alternative to listen to YouTube music on my RaspBerry Pi.

This project has two separate parts:
- the server side, which is a microservice running on the RaspBerry Pi, performing the URL extraction via youtube-dl and enqueueing the resulting address to a MPD server
- the client side, which is an Android application that is a valid share target for the YouTube app, and communicates with the server side

## Prerequisites ##

To build the server side, you need:
- a Linux platform (WSL not tested, sorry)
- a C++11 compiler (thread support is required)
- [CMake](https://www/cmake.org/)
- [libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/)
- [libmpdclient](https://www.musicpd.org/doc/libmpdclient/index.html)

To *run* the server side, your system must have youtube-dl installed and available in the path.

To build the client side, Android studio should be enough.


## Building the server side ##
To build the server side, you can use the usual commands:

    mkdir build
    cd build
    cmake ../server
    make

If all prerequisites are satisfied, you should end up with the *queueservice* executable in the build/apps folder.

This executable accepts the following parameters:

    -l [port]       : the port used by the microservice to accept requests from the client - defaults to 9090
    -s [ip address] : the IP address of the MPD server to enqueue elements to - defaults to 127.0.0.1
    -p [port]       : the port of the MPD server to enqueue elements to - defaults to 6600

A systemd service definition is provided, in case you want to run this at boot time - just copy server/qtube-server.service to /etc/systemd/system and adjust the paths and parameters to your liking.


## Building the client side ##
Use Android Studio to open the *client* folder; there should be no surprises here.

## How to use ##
Once you installed the client version on your Android device, you can open the QTube app to configure the connection parameters (address of the machine running the queueservice and port the service is listening on).
Then, use the YouTube app and search for the video file you want to enqueue to MPD.
When your desired video is open, click on the three dots menu and select "Share". 
A list of possible share target will appear, including QTube; if you select this, an enqueue request will be forwarded to the service.
