import os
import time
import picamera

video = True
numframes = 10

def img_filename(i):
    return '/mnt/camera/003-%06d.jpg' % i

for i in range(numframes):
    if os.path.isfile(img_filename(i)):
        os.remove(img_filename(i)) 

with picamera.PiCamera(sensor_mode=2) as camera:
    camera.resolution = (2592, 1944)
    camera.framerate = 10
    camera.iso = 800
    #camera.hflip = True
    #camera.vflip = True
    # Wait for the automatic gain control to settle
    time.sleep(2)
    # Now fix the values
    shutter = camera.exposure_speed
    #shutter = 20000
    camera.shutter_speed = shutter
    print "shutter_speed: {0}".format(shutter)
    camera.exposure_mode = 'off'
    g = camera.awb_gains
    camera.awb_mode = 'off'
    camera.awb_gains = g
    # Finally, take several photos with the fixed settings
    camera.capture_sequence([img_filename(i) for i in range(numframes)], use_video_port=video)
    #camera.capture(img_filename(0), use_video_port=False)
    #for i in range(numframes):
    #    print("taking %d" % i)
    #    camera.capture(img_filename(i), use_video_port=video)
