import socket
import time
import picamera

print "Hello"

with picamera.PiCamera() as camera:
    print "Camera is open"
    #camera.resolution = (2592, 1944)
    #camera.resolution = (1944, 1458)
    camera.resolution = (1296, 972)
    camera.framerate = 15

    print "Opening socket"

    sock = socket.socket()
    sock.bind(('0.0.0.0', 8004))
    sock.listen(0)

    listen_again = True
    while listen_again:
        listen_again = False
        print "Waiting for connection"

        # Accept a single connection and make a file-like object out of it
        connection = sock.accept()[0].makefile('wb')
        try:
            #camera.start_recording(connection, format='h264', quality=10)
            print "Recording"
            camera.start_recording(connection, format='mjpeg', quality=90)
            while True:
                print "Waiting..."
                camera.wait_recording(3600)
            #camera.stop_recording()
        except IOError as e:
            # We typically get socket.error when VLC disconnects: "[Errno 32] Broken pipe"
            print "Got IOError: ", e
            listen_again = True
        except Exception as e:
            print "Exception: ", type(e), " >> ", e

        if listen_again:
            # stop_recording is necessary, otherwise the camera port remained used,
            # and this try block is indeed necessary.
            # Seems like a sloppy implementation inside stop_recording.
            try:
                camera.stop_recording()
            except Exception as e:
                # We typically get "[Errno 104] Connection reset by peer" here
                print "Exception on stop_recording: ", e
        else:
            connection.close()

    print "closing socket"
    sock.close()
    print "gone"

