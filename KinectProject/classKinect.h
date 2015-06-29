#pragma once

template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL) {
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}


class kinect {
public:
	//Kinect states variable
	enum KinectStates {
		kIdle,
		kDetectGesture,
		kFaceDetect
	};
	void setKinectStates(KinectStates inputStates);
	KinectStates getKinectState(void);

private:
	KinectStates kinectStates = kIdle;
};