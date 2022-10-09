#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <iostream>
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <time.h>


int main() {
	
	cv::VideoCapture vid_stream;
	int apiID = cv::CAP_ANY;
    vid_stream.open(0, apiID);
    if(!vid_stream.isOpened()) {
		std::cerr << "Error opening video stream" << std::endl;
		return -1;
	}
	
	while (1) {
		time_t start = std::clock();
		cv::Mat frame;
		vid_stream >> frame;
		auto read_time = std::clock();
		std::cout << "Read time " << std::difftime(read_time, start)/CLOCKS_PER_SEC << std::endl;
		
		cv::Mat segmented;
		cv::inRange(frame, cv::Scalar(0, 0, 0), cv::Scalar(255, 255, 255), segmented);
		
		auto seg_time = std::clock();
		std::cout << "Segmenting time " << std::difftime(seg_time, read_time)/CLOCKS_PER_SEC << std::endl;
		std::cout << "Total time " << std::difftime(seg_time, start)/CLOCKS_PER_SEC << "\n------------" << std::endl;
		//cv::imshow("video", frame);
		
		// Press  ESC on keyboard to exit
		//char c = (char) cv::waitKey(25);
		//if (c==27) break;
	}
	
	vid_stream.release();
	
	cv::destroyAllWindows();

	
	return 0;
}
