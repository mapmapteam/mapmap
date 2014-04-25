gst-launch-0.10 -e shmsrc socket-path=/tmp/test is-live=1 ! \
    video/x-raw-yuv,format=\(fourcc\)I420,framerate=30/1,width=640,height=480 ! \
    xvimagesink 


