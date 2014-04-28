gst-launch-0.10 -v videotestsrc ! \
   'video/x-raw-yuv, width=640, height=480, framerate=30/1, format=(fourcc)I420' ! \
   shmsink socket-path=/tmp/test shm-size=10000000 wait-for-connection=0


