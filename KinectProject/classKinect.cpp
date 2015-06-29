#include "classKinect.h"

kinect::KinectStates kinect::getKinectState(void) {
	return kinectStates;
}

void kinect :: setKinectStates(KinectStates inputStates) {
	kinectStates = inputStates;
}