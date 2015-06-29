
#include "stdafx.h"
#include <Windows.h>
#include <Kinect.h>
#include <iostream>
#include <Kinect.VisualGestureBuilder.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <Serial.h>

using namespace std;

#define RX_BUFFSIZE 20

template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL) {
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

class robot {
public:
	//Robot states varriable
	enum RobotStates { 
		rIdle, 
		rInitial, 
		rSayHi 
	};
	void setRobotStates(RobotStates inputStates) {
		robotStates = inputStates;
	}
	RobotStates getRobotState(void) {
		return robotStates;
	}
	
private:
	RobotStates robotStates = rIdle;
};

class kinect {
public:
	//Kinect states variable
	enum KinectStates {
		kIdle,
		kDetectGesture,
		kFaceDetect
	};
	void setKinectStates(KinectStates inputStates) {
		kinectStates = inputStates;
	}
	KinectStates getKinectState(void) {
		return kinectStates;
	}

private:
	KinectStates kinectStates = kIdle;
};

int _tmain(int argc, _TCHAR* argv[])
{

	//--------------------initialize-----------------------
	if (argc != 2)
	{

		cout << "press enter to quit" << endl;
		char temp;
		cin >> temp;
		return 10;
	}

	try
	{
		wcout << "open port at : " << argv[1] << endl;
	}
	catch (const char *msg)
	{
		cout << msg << endl;
	}
	tstring commPortName(argv[1]);
	Serial serial(commPortName, 115200);
	cv::setUseOptimized(true);
	serial.flush();
	char cBuff[10] = "";
	char serialBuff[RX_BUFFSIZE] = "";

	// Kinect initial
	IKinectSensor* pSensor;
	HRESULT hResult = S_OK;
	hResult = GetDefaultKinectSensor(&pSensor);
	if (FAILED(hResult)) {
		std::cerr << "Error : GetDefaultKinectSensor" << std::endl;
		return -1;
	}

	hResult = pSensor->Open();
	if (FAILED(hResult)) {
		std::cerr << "Error : IKinectSensor::Open()" << std::endl;
		return -1;
	}

	// Source
	IColorFrameSource* pColorSource;
	hResult = pSensor->get_ColorFrameSource(&pColorSource);
	if (FAILED(hResult)) {
		std::cerr << "Error : IKinectSensor::get_ColorFrameSource()" << std::endl;
		return -1;
	}

	IBodyFrameSource* pBodySource;
	hResult = pSensor->get_BodyFrameSource(&pBodySource);
	if (FAILED(hResult)) {
		std::cerr << "Error : IKinectSensor::get_BodyFrameSource()" << std::endl;
		return -1;
	}



	// Reader
	IColorFrameReader* pColorReader;
	hResult = pColorSource->OpenReader(&pColorReader);
	if (FAILED(hResult)) {
		std::cerr << "Error : IColorFrameSource::OpenReader()" << std::endl;
		return -1;
	}

	IBodyFrameReader* pBodyReader;
	hResult = pBodySource->OpenReader(&pBodyReader);
	if (FAILED(hResult)) {
		std::cerr << "Error : IBodyFrameSource::OpenReader()" << std::endl;
		return -1;
	}



	// Description
	IFrameDescription* pDescription;
	hResult = pColorSource->get_FrameDescription(&pDescription);
	if (FAILED(hResult)) {
		std::cerr << "Error : IColorFrameSource::get_FrameDescription()" << std::endl;
		return -1;
	}

	int width = 0;
	int height = 0;
	pDescription->get_Width(&width); // 1920
	pDescription->get_Height(&height); // 1080
	unsigned int bufferSize = width * height * 4 * sizeof(unsigned char);

	cv::Mat bufferMat(height, width, CV_8UC4);
	cv::Mat bodyMat(height / 2, width / 2, CV_8UC4);
	cv::namedWindow("Camera");

	// Color Table
	cv::Vec3b color[BODY_COUNT];
	color[0] = cv::Vec3b(255, 0, 0);
	color[1] = cv::Vec3b(0, 255, 0);
	color[2] = cv::Vec3b(0, 0, 255);
	color[3] = cv::Vec3b(255, 255, 0);
	color[4] = cv::Vec3b(255, 0, 255);
	color[5] = cv::Vec3b(0, 255, 255);

	// Coordinate Mapper
	ICoordinateMapper* pCoordinateMapper;
	hResult = pSensor->get_CoordinateMapper(&pCoordinateMapper);
	if (FAILED(hResult)) {
		std::cerr << "Error : IKinectSensor::get_CoordinateMapper()" << std::endl;
		return -1;
	}

	IVisualGestureBuilderFrameSource* pGestureSource[BODY_COUNT];
	IVisualGestureBuilderFrameReader* pGestureReader[BODY_COUNT];
	for (int count = 0; count < BODY_COUNT; count++) {
		// Source
		hResult = CreateVisualGestureBuilderFrameSource(pSensor, 0, &pGestureSource[count]);
		if (FAILED(hResult)) {
			std::cerr << "Error : CreateVisualGestureBuilderFrameSource()" << std::endl;
			return -1;
		}

		// Reader
		hResult = pGestureSource[count]->OpenReader(&pGestureReader[count]);
		if (FAILED(hResult)) {
			std::cerr << "Error : IVisualGestureBuilderFrameSource::OpenReader()" << std::endl;
			return -1;
		}
	}

	// Create Gesture Dataase from File (*.gba)
	IVisualGestureBuilderDatabase* pGestureDatabase;
	hResult = CreateVisualGestureBuilderDatabaseInstanceFromFile(L"HandUp.gba"/*L"Swipe.gba"*/, &pGestureDatabase);
	if (FAILED(hResult)) {
		std::cerr << "Error : CreateVisualGestureBuilderDatabaseInstanceFromFile()" << std::endl;
		return -1;
	}

	// Add Gesture
	UINT gestureCount = 0;
	hResult = pGestureDatabase->get_AvailableGesturesCount(&gestureCount);
	if (FAILED(hResult) || !gestureCount) {
		std::cerr << "Error : IVisualGestureBuilderDatabase::get_AvailableGesturesCount()" << std::endl;
		return -1;
	}

	IGesture* pGesture;
	hResult = pGestureDatabase->get_AvailableGestures(gestureCount, &pGesture);
	if (SUCCEEDED(hResult) && pGesture != nullptr) {
		for (int count = 0; count < BODY_COUNT; count++) {
			hResult = pGestureSource[count]->AddGesture(pGesture);
			if (FAILED(hResult)) {
				std::cerr << "Error : IVisualGestureBuilderFrameSource::AddGesture()" << std::endl;
				return -1;
			}

			hResult = pGestureSource[count]->SetIsEnabled(pGesture, true);
			if (FAILED(hResult)) {
				std::cerr << "Error : IVisualGestureBuilderFrameSource::SetIsEnabled()" << std::endl;
				return -1;
			}
		}
	}

	kinect vision;
	robot pili;




	//----------------------Main loop--------------------------
	while (1) {
		//Kinect module
		// always get color frame
		IColorFrame* pColorFrame = nullptr;
		hResult = pColorReader->AcquireLatestFrame(&pColorFrame);
		if (SUCCEEDED(hResult)) {
			hResult = pColorFrame->CopyConvertedFrameDataToArray(bufferSize, reinterpret_cast<BYTE*>(bufferMat.data), ColorImageFormat::ColorImageFormat_Bgra);
			if (SUCCEEDED(hResult)) {
				cv::resize(bufferMat, bodyMat, cv::Size(), 0.5, 0.5);
			}
		}
		SafeRelease(pColorFrame);
		IBodyFrame* pBodyFrame = nullptr;
		// Kinect state machine
		switch(vision.getKinectState()) {
		case kinect::kIdle:
			break;
		case kinect::kDetectGesture:
			hResult = pBodyReader->AcquireLatestFrame(&pBodyFrame);
			if (SUCCEEDED(hResult)) {
				IBody* pBody[BODY_COUNT] = { 0 };
				hResult = pBodyFrame->GetAndRefreshBodyData(BODY_COUNT, pBody);
				if (SUCCEEDED(hResult)) {
					for (int count = 0; count < BODY_COUNT; count++) {
						BOOLEAN bTracked = false;
						hResult = pBody[count]->get_IsTracked(&bTracked);
						if (SUCCEEDED(hResult) && bTracked) {
							// Joint
							Joint joint[JointType::JointType_Count];
							hResult = pBody[count]->GetJoints(JointType::JointType_Count, joint);
							if (SUCCEEDED(hResult)) {
								// show joints
								for (int type = 0; type < JointType::JointType_Count; type++) {
									ColorSpacePoint colorSpacePoint = { 0 };
									pCoordinateMapper->MapCameraPointToColorSpace(joint[type].Position, &colorSpacePoint);
									int x = static_cast<int>(colorSpacePoint.X);
									int y = static_cast<int>(colorSpacePoint.Y);
									if ((x >= 10) && (x < width - 10) && (y >= 10) && (y < height - 10)) {
										cv::circle(bufferMat, cv::Point(x, y), 5, static_cast< cv::Scalar >(color[count]), -1, CV_AA);
									}
								}
							}

							// Set TrackingID to Detect Gesture
							UINT64 trackingId = _UI64_MAX;
							hResult = pBody[count]->get_TrackingId(&trackingId);
							if (SUCCEEDED(hResult)) {
								pGestureSource[count]->put_TrackingId(trackingId);
							}
						}
					}
					cv::resize(bufferMat, bodyMat, cv::Size(), 0.5, 0.5);
				}
				for (int count = 0; count < BODY_COUNT; count++) {
					SafeRelease(pBody[count]);
				}
			}
			SafeRelease(pBodyFrame);

			// Detect Gesture
			std::system("cls");
			for (int count = 0; count < BODY_COUNT; count++) {
				IVisualGestureBuilderFrame* pGestureFrame = nullptr;
				hResult = pGestureReader[count]->CalculateAndAcquireLatestFrame(&pGestureFrame);
				if (SUCCEEDED(hResult) && pGestureFrame != nullptr) {
					BOOLEAN bGestureTracked = false;
					hResult = pGestureFrame->get_IsTrackingIdValid(&bGestureTracked);
					if (SUCCEEDED(hResult) && bGestureTracked) {
						// Discrete Gesture (Sample HandUp.gba is Action to Hand Up above the head.)
						IDiscreteGestureResult* pGestureResult = nullptr;
						hResult = pGestureFrame->get_DiscreteGestureResult(pGesture, &pGestureResult);
						if (SUCCEEDED(hResult) && pGestureResult != nullptr) {
							BOOLEAN bDetected = false;
							hResult = pGestureResult->get_Detected(&bDetected);
							if (SUCCEEDED(hResult) && bDetected) {
								std::cout << "Detected Gesture" << std::endl;
								// if success detecting gesture, change state.
								vision.setKinectStates(kinect::kIdle);	
								pili.setRobotStates(robot::rSayHi);
							}
						}

						/*// Continuous Gesture (Sample Swipe.gba is Action to Swipe the hand in horizontal direction.)
						IContinuousGestureResult* pGestureResult = nullptr;
						hResult = pGestureFrame->get_ContinuousGestureResult( pGesture, &pGestureResult );
						if( SUCCEEDED( hResult ) && pGestureResult != nullptr ){
						float progress = 0.0f;
						hResult = pGestureResult->get_Progress( &progress );
						if( SUCCEEDED( hResult ) ){
						std::cout << "Progress: " + std::to_string( progress ) << std::endl;
						}
						}*/

						SafeRelease(pGestureResult);
					}
				}
				SafeRelease(pGestureFrame);
			}
			break;
		case kinect::kFaceDetect:
			break;
		default:
			break;
		}
		// show image
		cv::imshow("Camera", bodyMat);
		if (cv::waitKey(10) == VK_ESCAPE) {
			break;
		}
		else if (cv::waitKey(10) == VK_SPACE) {	// press space key and change kinectStates to detect gesture
			vision.setKinectStates(kinect::kDetectGesture);
			pili.setRobotStates(robot::rInitial);
		}

		// Robot state machine
		switch (pili.getRobotState()) {
		case robot::rIdle:
			/*do
			{
				serial.read(serialBuff, RX_BUFFSIZE);
				Sleep(100);
			} while (serialBuff[0] == '\0');.
			cout << "serial port read: " << serialBuff << endl;
			serialBuff[0] = '\0';*/
			//serial.write(cBuff);
			break;
		case robot::rInitial:
			serial.write("r");
			Sleep(5000);
			serial.write("0");
			Sleep(500);
			pili.setRobotStates(robot::rIdle);
			break;
		case robot::rSayHi:
			serial.write("b");
			Sleep(5000);
			serial.write("j");
			Sleep(16000);
			serial.write("g");
			Sleep(3000);
			// continuous detect gesture
			vision.setKinectStates(kinect::kDetectGesture);
			pili.setRobotStates(robot::rIdle);
			break;
		default:
			break;
		}
		



		
	}
	//--------------------Release Memory------------------------
	SafeRelease(pColorSource);
	SafeRelease(pBodySource);
	SafeRelease(pColorReader);
	SafeRelease(pBodyReader);
	SafeRelease(pDescription);
	SafeRelease(pCoordinateMapper);
	for (int count = 0; count < BODY_COUNT; count++) {
		SafeRelease(pGestureSource[count]);
		SafeRelease(pGestureReader[count]);
	}
	SafeRelease(pGestureDatabase);
	SafeRelease(pGesture);
	if (pSensor) {
		pSensor->Close();
	}
	SafeRelease(pSensor);
	cv::destroyAllWindows();

	return 0;
}
