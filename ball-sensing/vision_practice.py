import cv2
import datetime
import numpy as np
import time
import imutils

vid = cv2.VideoCapture(0)
#vid.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('M', 'J', 'P', 'G'))
#vid.set(cv2.CAP_PROP_FPS, 30) 

H_min = 1000
H_max = 0
S_min = 1000
S_max = 0
V_min = 1000
V_max = 0

current_center = (0, 0)
prev_center = (0, 0)
current_time = 0.0
prev_time = 0.0
total_time = 0.0
max_time = 0.0
time_counts = 0.0
above_33 = 0.0

while(True):
      
    # Capture the video frame
    # by frame
    start_time = time.perf_counter()
    ret, frame = vid.read()
    print("Read time: ", time.perf_counter() - start_time)

    #Enables sampling color values of a pixel
   #def mouseRGB(event,x,y,flags,param):
    #   if event == cv2.EVENT_LBUTTONDOWN: #checks mouse left button down condition
     #      colorsB = frame[y,x,0]
      #     colorsG = frame[y,x,1]
       #    colorsR = frame[y,x,2]
        #   colors = frame[y,x]
        #   print("B: ",colorsB)
        #   print("G: ",colorsG)
        #   print("R: ",colorsR)
        #   print("BGR Format: ",colors)
        #   print("Coordinates of pixel: X: ", x,"Y: ", y)

    # convert to hsv colorspace
    # lower bound and upper bound for desired color
    lower_bound = np.array([50, 90, 180])   
    upper_bound = np.array([90, 120, 225])

    # find the colors within the boundaries
    mask = cv2.inRange(frame, lower_bound, upper_bound)
    
    segment_time = time.perf_counter()
    print("Segmenting time: ", segment_time - start_time)
    
    # Segment only the detected region
    #segmented_img = cv2.bitwise_and(frame, frame, mask=mask)
    #segmented_img = cv2.bitwise_not(segmented_img, segmented_img, mask=mask)
    
    mask_time = time.perf_counter()

    #get the contour and then draw a rectangle if a contour is found
    contours, hierarchy = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    #output = cv2.drawContours(segmented_img, contours, -1, (0, 0, 255), 3)
    found_frame = False
    for contour in contours:
        if cv2.contourArea(contour) > 25:
            found_frame = True
            x, y, w, h = cv2.boundingRect(contour)
            #cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 0, 255), 2)
            prev_center= current_center
            current_center = (x + int(w / 2), y + int(h / 2))
            #cv2.arrowedLine(frame, (prev_center), (current_center), (0, 255, 0), 2)
            break

    contour_time = time.perf_counter()
    #print("Contour time: ", contour_time - mask_time)
    print("Total time: ", contour_time - start_time, found_frame, "\n-------------")
    
    total_time += (contour_time - start_time)
    max_time = max((contour_time - start_time), max_time)
    time_counts += 1
    if (contour_time - start_time > 0.0333): above_33 += 1
        # Display the resulting frame
    #cv2.imshow('video', frame)
    #cv2.imshow('segmented',segmented_img)
   #cv2.setMouseCallback('video',mouseRGB)

      
    # the 'q' button is set as the
    # quitting button you may use any
    # desired button of your choice
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
  
# After the loop release the cap object
vid.release()
# Destroy all the windows
cv2.destroyAllWindows()
print("Average Time: ", total_time / time_counts)
print("Max Time: ", max_time)
print("Percent longer than 33ms: ", above_33 / time_counts * 100)