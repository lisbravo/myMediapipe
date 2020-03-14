

$ bazel build -c opt --define MEDIAPIPE_DISABLE_GPU=1 \
    mediapipe/examples/desktop/hand_landmark:hand_landmark_cpu__tflite

# It should print:
# Target //mediapipe/examples/desktop/simpleIO:simple_io_tflite up-to-date:
#   bazel-bin/mediapipe/examples/desktop/simpleIO/simple_io_tflite
# INFO: Elapsed time: 36.417s, Critical Path: 23.22s
# INFO: 711 processes: 710 linux-sandbox, 1 local.
# INFO: Build completed successfully, 734 total actions

$ export GLOG_logtostderr=1



# INPUT=  Stream , OUTPUT=screen

$ bazel-bin/mediapipe/examples/desktop/hand_landmark/hand_landmark_cpu__tflite \
    --calculator_graph_config_file=mediapipe/graphs/hand_tracking/hand_tracking_desktop.pbtxt \
    --input_side_packets=input_video_path=rtp://0.0.0.0:5000