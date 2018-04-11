# LinuxCameraApp
This Camera app is built using GTK and Gstreamer frameworks on Linux.
In order to build this App;
1. Install GTK using this command: sudo apt-get install libgtk-3-dev
2. Install all necessary Gstreamer packages using this command: sudo apt-get install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools
3. Compile Camera.c file using this command: cc Camera.c -o Camera `pkg-config --cflags --libs gtk+-3.0 gstreamer-1.0`
4. Run executable using this command: ./Camera

