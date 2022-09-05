from turtle import width
import cv2 as cv
from mss import mss
from PIL import Image
import numpy as np
import serial
import time



ARDUINO= serial.Serial("COM3", 115200, timeout=3)

NUM_LEDS = 16

bounding_box = {'top': 0, 'left': 0, 'width': 1920, 'height': 1080}


def screenCapture():
    run = True
    sct = mss()
    #cap = cv.VideoCapture(0)

    with mss() as sct:
        while(run):
            
            screenshot = sct.grab(bounding_box)         #Aproximadamente 30fps
            screenshot = Image.frombytes(
                'RGB', 
                (screenshot.width, screenshot.height), 
                screenshot.rgb, 
            )
            screenshot = np.array(screenshot)
            screenshot = cv.cvtColor(screenshot, cv.COLOR_RGB2BGR)

            X_MAX, Y_MAX, channels = screenshot.shape

            
            X_MAX_div = X_MAX//(NUM_LEDS+1)           #X_MAX // (NUM_LEDS + 1)

            r_left = 0
            g_left = 0
            b_left = 0
            r_right = 0
            g_right = 0
            b_right = 0
            cont = 0

            ARDUINO.write(b'w')


            for i in range(0, X_MAX):
                if cont >= X_MAX_div:
                    b_left = b_left //((Y_MAX//4)*X_MAX_div)
                    g_left = g_left //((Y_MAX//4)*X_MAX_div)
                    r_left = r_left //((Y_MAX//4)*X_MAX_div)

                    b_right = b_right //((Y_MAX//4)*X_MAX_div)
                    g_right = g_right //((Y_MAX//4)*X_MAX_div)
                    r_right = r_right //((Y_MAX//4)*X_MAX_div)

                    sendColour(str(r_left)+"r", str(g_left)+"g", str(b_left)+"b",
                        str(r_right)+"p", str(g_right)+"l", str(b_right)+"k")
                
                
                    b_left = 0
                    g_left = 0
                    r_left = 0

                    b_right = 0
                    g_right = 0
                    r_right = 0

                    cont = 0

                for j in range(0, Y_MAX//4):
                    b_left += screenshot.item(i,j,0)
                    g_left += screenshot.item(i,j,1)
                    r_left += screenshot.item(i,j,2)
                
                for j in range(3*Y_MAX//4, Y_MAX):
                    b_right += screenshot.item(i,j,0)
                    g_right += screenshot.item(i,j,1)
                    r_right += screenshot.item(i,j,2)


                cont += 1

                #cv.imshow('Screen Capture', screenshot)
                
            #time.sleep(0.100)

            if cv.waitKey(1) == ord('a'):
                cv.destroyAllWindows()
                run = False


def sendColour(r_left, g_left, b_left, r_right, g_right, b_right):
    

    ARDUINO.write(r_left.encode())
    ARDUINO.write(g_left.encode())
    ARDUINO.write(b_left.encode()) 
    
    
    ARDUINO.write(r_right.encode())
    ARDUINO.write(g_right.encode())
    ARDUINO.write(b_right.encode()) 
    
    

if __name__ == "__main__":
    time.sleep(3)
    screenCapture()

