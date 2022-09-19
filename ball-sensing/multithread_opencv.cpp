//https://stackoverflow.com/questions/62069386/getting-opencv-headers-to-work-correctly-on-install-in-linux
//resolves how to fix includes

#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>
#include <iostream>
#include <ctime>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

class BallSensing {
    public:
        cv::Mat current_frame;
        cv::VideoCapture vid_stream;
        std::mutex the_mutex;
        std::condition_variable cond_var;
        bool frame_ready;
        
        //Default constructor
        BallSensing() {
            frame_ready = false;
        }

        //Every interval (defined by mod statement) read in a frame. If a frame is successfully read, notify the condition variable so that processing can happen
        void save_frame() {
            // std::cout << "Begin save_frame" << std::endl;

            while(1) {
                //lock the object as we read in video
                std::lock_guard<std::mutex> guard(the_mutex);

                //check the time in ms
                auto current_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

                //if enough time has passed and we haven't already read a frame, read one
                if (current_time % 1000 == 0 && !frame_ready) {
                    cv::Mat frame;
                    vid_stream.read(frame);
                    // check if we succeeded
                    if (frame.empty()) {
                        std::cerr << "ERROR! blank frame grabbed" << std::endl;
                    } else {
                        current_frame = frame; //save the frame inside the object
                    }   
                    // std::cout << "Read a frame!" << std::endl;
                    frame_ready = true;
                    cond_var.notify_one(); //notify the condition variable
                }
            }  
        }

        //Initialize VideoCapture object and start reading in video
        bool init_video() {
            int apiID = cv::CAP_ANY;
            vid_stream.open(0, apiID);
            return vid_stream.isOpened();
        }

        //Wait for a thread to be read in. Once it is read process then go back to waiting.
        void process_frame() {
            // std::cout << "Begin process_frame" << std::endl;
            
            while(1) {
            std::unique_lock<std::mutex> mlock(the_mutex); //try to take the lock
            while (!frame_ready) {
                cond_var.wait(mlock); //wait for a notification from the condvar
            }
            // std::cout << "Processed a frame!" << std::endl;

            //Do some mindless work for testing. This is where image processing happens
            for (int i = 0; i < 100000; i++) {
                int a = 2; 
            }
            imshow("Video", current_frame);
            if (cv::waitKey(5) >= 0) {
                return;
            }
            frame_ready = false; //signal that another frame should be read
            }
        }
};

int main() {
    BallSensing ball_sensing;
    if (!ball_sensing.init_video()) return 0; //if the camera didn't open, stop
    std::thread thread_1(&BallSensing::save_frame, &ball_sensing);
    std::thread thread_2(&BallSensing::process_frame, &ball_sensing);
    thread_2.join();
    thread_1.join();
    return 0;
}