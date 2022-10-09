thread_flags=-pthread
linker_flags=-lopencv_core -lopencv_videoio -lopencv_highgui
CXX=clang++
multithread:
	$(CXX) multithread_opencv.cpp $(thread_flags) $(linker_flags)

singlethread:
	$(CXX) single_thread.cpp $(thread_flags) $(linker_flags)
