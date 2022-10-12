import cv2
import numpy as np
import time

vid = cv2.VideoCapture(0)

sum_b = 0
sum_g = 0
sum_r = 0
max_b = 0
min_b = 255
max_g = 0
min_g = 255
max_r = 0
min_r = 255
num_points = 0

while(True):
    
    ret, frame = vid.read()
    
    def mouseBGR(event, x, y, flags, param):
        if event == cv2.EVENT_LBUTTONDOWN:
            global sum_b, max_b, min_b, sum_g, max_g, min_g, sum_r, max_r, min_r, num_points
            
            colorsB = frame[y, x, 0]
            colorsG = frame[y, x, 1]
            colorsR = frame[y, x, 2]
            colors = frame[y, x]
            print("BGR Colors: ", colors)
            print("At pixel: x: ", x, "y: ", y)
            
            sum_b += colorsB
            max_b = max(colorsB, max_b)
            min_b = min(colorsB, min_b)
            
            sum_g += colorsG
            max_g = max(colorsG, max_g)
            min_g = min(colorsG, min_g)
            
            sum_r += colorsR
            max_r = max(colorsR, max_r)
            min_r = min(colorsR, min_r)
            
            num_points += 1

            
    
    cv2.imshow('video', frame)
    cv2.setMouseCallback('video', mouseBGR)
    
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

vid.release()
print("B avg: ", float(sum_b)/num_points, "B range:", (min_b, max_b))
print("G avg: ", float(sum_g)/num_points, "G range:", (min_g, max_g))
print("R avg: ", float(sum_r)/num_points, "R range:", (min_r, max_r))
cv2.destroyAllWindows()