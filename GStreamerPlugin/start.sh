export GST_PLUGIN_PATH=/usr/local/lib/gstreamer-1.0
make
make install
#gst-launch-1.0 v4l2src device=/dev/video0 ! ocv ! autovideosink
gst-launch-1.0 -v -m fakesrc ! ocv ! fakesink silent=TRUE
