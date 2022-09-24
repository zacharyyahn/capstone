import cv2
import datetime
import numpy as np
import time

vid = cv2.VideoCapture(0)

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

  
while(True):
      
    # Capture the video frame
    # by frame
    ret, frame = vid.read()

    #Enables sampling color values of a pixel
    def mouseRGB(event,x,y,flags,param):
        if event == cv2.EVENT_LBUTTONDOWN: #checks mouse left button down condition
            colorsB = frame[y,x,0]
            colorsG = frame[y,x,1]
            colorsR = frame[y,x,2]
            colors = frame[y,x]
            print("B: ",colorsB)
            print("G: ",colorsG)
            print("R: ",colorsR)
            print("BGR Format: ",colors)
            print("Coordinates of pixel: X: ", x,"Y: ", y)

    # convert to hsv colorspace
    # lower bound and upper bound for desired color
    lower_bound = np.array([170, 245, 245])   
    upper_bound = np.array([185, 255, 255])

    # find the colors within the boundaries
    mask = cv2.inRange(frame, lower_bound, upper_bound)

    #Do some denoising:
    #define kernel size  
    kernel = np.ones((7,7),np.uint8)
    # Remove unnecessary noise from mask
    mask = cv2.morphologyEx(mask, cv2.MORPH_CLOSE, kernel)
    mask = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
    
    # Segment only the detected region
    segmented_img = cv2.bitwise_and(frame, frame, mask=mask)
    segmented_img = cv2.bitwise_not(segmented_img, segmented_img, mask=mask)

    #get the contour and then draw a rectangle if a contour is found
    contours, hierarchy = cv2.findContours(mask.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    output = cv2.drawContours(segmented_img, contours, -1, (0, 0, 255), 3)
    if len(contours) > 0:
        max_contour = 0
        max_area = 0
        for contour in contours:
            if cv2.contourArea(contour) > max_area:
                max_area = cv2.contourArea(contour)
                max_contour = contour

        x, y, w, h = cv2.boundingRect(max_contour)
        cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 0, 255), 2)

        prev_center = current_center
        current_center = (x + int(w / 2), y + int(h / 2))
        cv2.arrowedLine(frame, (prev_center), (current_center), (0, 255, 0), 2)

        prev_time = current_time
        current_time = time.time()
        # print("Current Time: ", current_time, "Previous Time: ", prev_time, "Diff: ", current_time - prev_time)
        # print("Current Freq: ", (float(1) / (current_time - prev_time)))


    
    # Display the resulting frame
    cv2.imshow('video', frame)
    cv2.imshow('segmented',output)
    cv2.setMouseCallback('video',mouseRGB)

      
    # the 'q' button is set as the
    # quitting button you may use any
    # desired button of your choice
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
  
# After the loop release the cap object
vid.release()
# Destroy all the windows
cv2.destroyAllWindows()