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
	start_time = time.perf_counter()
	ret, frame = vid.read()
	
	greyframe = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
	
	circles = cv2.HoughCircles(greyframe, cv2.HOUGH_GRADIENT, 1, 20, param1=50, param2=30, minRadius=0, maxRadius=20)
	for i in circles[0]:
		print(i)
		x = int(i[0])
		y = int(i[1])
		cv2.circle(greyframe, (x, y), int(i[2]), (0, 255, 0), 2)
		cv2.circle(greyframe, (x, y), 2, (0, 0, 255), 3)
		
	end_time = time.perf_counter()
	print("Time taken: ", end_time - start_time, "\n----------------")	
	cv2.imshow('video', greyframe)
	if cv2.waitKey(1) & 0xFF == ord('q'):
		break

vid.release()
cv2.destroyAllWindows()
