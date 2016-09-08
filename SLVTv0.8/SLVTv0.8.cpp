// SLT_v0.1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "KinectSensor.h"
#include <string>
#include <utility>
#include <vector>
#include "HandExtraction.h"

#include "JavaMachine.h"

#define BODY_COUNT	6
#define DEPTH_THRESHOLD 80  //In milimeters (Kinect maximun range is 8000->8m)
#define HAND_WIDTH_HEIGTH 75
#define KINECT_DEPTH_WIDTH 512
#define KINECT_DEPTH_HEIGTH 424
#define KINECT_DEPTH_MAX 8000
#define DISPLACEMENT 15

/* Convert the Depth data from the kinect to a OpenCV Mat format of type CV_8UC3 */
cv::Mat convert_Depth2Mat(IDepthFrame * depthFrame){
	HRESULT hr;

	// Frame description //
	IFrameDescription * frameDesc; depthFrame->get_FrameDescription(&frameDesc);
	int height, width;
	frameDesc->get_Height(&height); frameDesc->get_Width(&width);
	int frame_size = width * height;

	cv::Mat img = cv::Mat::zeros(height, width, CV_8UC3);

	// Store the frame in the buffer //
	UINT16 * pBuffer = NULL;
	pBuffer = new UINT16[frame_size];
	hr = depthFrame->CopyFrameDataToArray(frame_size, pBuffer);

	if (SUCCEEDED(hr)){
		// Scale the data //
		USHORT minDepth; depthFrame->get_DepthMinReliableDistance(&minDepth);
		USHORT maxDepth; depthFrame->get_DepthMaxReliableDistance(&maxDepth);
		double scale = 255.0 / (maxDepth - minDepth);

		cv::Mat t1 = cv::Mat(height, width, CV_16U, pBuffer);
		cv::Mat t2 = cv::Mat(height, width, CV_8U, pBuffer);
		t1.convertTo(t2, CV_8U, scale); // It is not converting to the correct 3 channel type
		for (int i = 0; i < height*width; i++){
			uchar value = t2.at<uchar>(i);
			img.at<cv::Vec3b>(i).val[0] = value;
			img.at<cv::Vec3b>(i).val[1] = value;
			img.at<cv::Vec3b>(i).val[2] = value;
		}

		t1.release();
		t2.release();
	}

	return img;
}


cv::Mat extract_depth(cv::Mat frame, int minDepth, int maxDepth){

	int rows = frame.rows;
	int cols = frame.cols;

	int ch = frame.channels();
	int ty = frame.type();
	size_t si = frame.elemSize();

	cv::Mat extracted_depth = cv::Mat::zeros(rows, cols, CV_8UC3);

	// Filter the depth //
	for (int i = 0; i < rows; i++){
		for (int j = 0; j < cols; j++){
			int value = frame.at<cv::Vec3b>(i, j).val[0];
			if (j >= 170)
				j = j;
			if (value > minDepth && value < maxDepth){
				extracted_depth.at<cv::Vec3b>(i, j).val[0] = value;
				extracted_depth.at<cv::Vec3b>(i, j).val[1] = value;
				extracted_depth.at<cv::Vec3b>(i, j).val[2] = value;
			}
			else {
				extracted_depth.at<cv::Vec3b>(i, j).val[0] = 0;
				extracted_depth.at<cv::Vec3b>(i, j).val[1] = 0;
				extracted_depth.at<cv::Vec3b>(i, j).val[2] = 0;
			}
		}
	}

	return extracted_depth;
}

cv::Mat filter_color(cv::Mat frame, cv::Scalar low_value, cv::Scalar high_value){
	cv::Mat hsv_image = cv::Mat::ones(frame.rows, frame.cols, CV_8UC3);
	cv::cvtColor(frame, hsv_image, cv::COLOR_BGR2HSV);

	cv::Mat extracted_color;
	cv::inRange(hsv_image, low_value, high_value, extracted_color);

	hsv_image.release();
	return extracted_color;
}

int _tmain(int argc, _TCHAR* argv[])
{
	
	
	// Initialize Java Virtual Machine
	JavaMachine jvm = JavaMachine();
	//jvm.initialize();
	

	// Initialize the Kinect sensor and the readers
	KinectSensor kinectSensor = KinectSensor();
	IBodyFrame * bodyFrame;
	IDepthFrame * depthFrame;
	IColorFrame * colorFrame;
	IDepthFrameReader * depthreader = kinectSensor.getDepthFrameReader();
	IColorFrameReader * colorreader = kinectSensor.getColorFrameReader();
	IBodyFrameReader * bodyreader = kinectSensor.getBodyFrameReader();
	ICoordinateMapper * coordinateMapper = kinectSensor.getCoordinateMapper();


	std::ofstream f;
	f.open("tmp_values.csv", std::ios::app);

	int t0 = std::time(nullptr);
	
	

	while (1){
		//system("CLS"); // Si casca el programa meter esto dentro del while
		IBody* ppBodies[BODY_COUNT] = { 0 };

		// Get the frames //
		HRESULT hr = bodyreader->AcquireLatestFrame(&bodyFrame);
		if (!SUCCEEDED(hr))
			continue;
		hr = depthreader->AcquireLatestFrame(&depthFrame);
		//hr = colorreader->AcquireLatestFrame(&colorFrame);

		//std::cout << SUCCEEDED(hr) << "," << (bodyFrame == NULL) << std::endl;
		
		if (!SUCCEEDED(hr) || bodyFrame == NULL)
			continue;
	
		hr = bodyFrame->GetAndRefreshBodyData(_countof(ppBodies), ppBodies);
		bodyFrame->Release();
		bodyFrame = NULL;


		if (!SUCCEEDED(hr))
			continue;

		// Process bodies //
		Joint joints[JointType_Count];
		for (int i = 0; i < BODY_COUNT; i++){

			IBody * pBody = ppBodies[i];
			if (pBody){
				BOOLEAN bTracked = false;
				hr = pBody->get_IsTracked(&bTracked);
				
				if (SUCCEEDED(hr) && bTracked){
					
					DepthSpacePoint * jointPoints[JointType_Count];

					hr = pBody->GetJoints(_countof(joints), joints);
					Joint spineBase = joints[JointType_SpineBase];

					/* //Solo para coger los joints, no para la imagen depth
					if (SUCCEEDED(hr)){
						for (int i = 0; i < _countof(joints); i++){
							Joint joint = joints[i];
							//joint.Position.X -= spineBase.Position.X;
							//joint.Position.Y -= spineBase.Position.Y;
							//joint.Position.Z -= spineBase.Position.Z;
							/*std::cout << "[" << joint.JointType << "] - ( " << joint.Position.X << " , "
							<< joint.Position.Y << " , "
							<< joint.Position.Z << " )\n";
							//	f << frame << ";" << i << "; " << joint.Position.X << "; " << joint.Position.Y << "; " << joint.Position.Z << "; " << signName << "\n";
						}
					}*/

					cv::Mat rightHand = extract_hand_region(depthFrame, joints[JointType_HandRight], bTracked);
					//cv::Mat fourier = getFourierTransformation(rightHand);
					/*
					short int max = 0;
					for (int i = 0; i < rightHand.rows; i++)
						for (int j = 0; j < rightHand.cols; j++)
						{
							if (max < rightHand.at<short int>(i, j))
								max = rightHand.at<short int>(i, j);
						}
						*/
					//std::cout << "Max: " << max << std::endl;
					//Sleep(1000);
					
					int k = 0;
					int avg = 0;
					for (int i = 0; i < rightHand.rows; i++){
						for (int j = 0; j < rightHand.cols; j++)
						{
							if (rightHand.at<short int>(i, j) != 0){
								k++;
								avg += rightHand.at<short int>(i, j);
							}
							rightHand.at<short int>(i, j) = rightHand.at<short int>(i, j) * 10;
						}
					}
					k = k == 0 ? 1 : k;
					avg = avg / k;
					
					
					// Write the values into a csv file
					/*
					for (int i = 0; i < rightHand.rows; i++){
						for (int j = 0; j < rightHand.cols; j++)
						{
							k++;
							int v = rightHand.at<short int>(i, j);
							if (v != 0)
								v -= avg;
							f << v << ";";
						}
					}
					f << "\n";
					*/
					
					

					/*
					// Get the double array of the values
					std::vector<double> raw_instance = std::vector<double>(rightHand.rows*rightHand.cols);
					for (int i = 0; i < rightHand.rows; i++)
						for (int j = 0; j < rightHand.cols; j++)
						{
							double v = rightHand.at<short int>(i, j);
							if (v != 0)
								v -= avg;
							raw_instance[i*rightHand.cols + j] = rightHand.at<short int>(i, j);
						}
						
						
					
					//double * raw_instance = (double *)malloc(2 * sizeof(double));
					const char * instance_class = jvm.classify(raw_instance);
					std::cout << "Class: " << instance_class << std::endl;
					//Sleep(50);
					*/

					//cv::Mat leftHand = extract_hand_region(depthFrame, joints[JointType_HandLeft], bTracked);

					//cv::resize(colorFrame, colorFrame, cv::Size(KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGTH));
					//cv::Mat color = extract_hand_region(colorFrame, joints[JointType_HandRight], bTracked);
					//cv::Mat colorHand = extract_colorHand_from_depthHand(color, rightHand);
					//cv::Mat bordersColor = extract_borders(color, 50, 100);



					//imshow("Depth", extractedDepth);
					//cv::waitKey(30);

					//cv::FileStorage file("points.dat", cv::FileStorage::WRITE);
					//file << "depth_hand" << rightHand;
					//file.release();

					//cv::Mat bordersRightHand;
					//bordersRightHand = extract_borders(rightHand, 10, 25);

					//imshow("Borders", bordersRightHand);

					//cv::Mat filteredColor = filter_color(color, cv::Scalar(0, 0, 0), cv::Scalar(50, 50, 50));
					//cv::Mat bordersColor = extract_borders(filteredColor, 50, 100);

					imshow("Right Hand", rightHand);  
					cv::waitKey(30);
					//imshow("Left Hand", leftHand);
					//cv::waitKey(30);

					//imshow("Fourier", fourier);
					cv::waitKey(30);

					// Free opencv mat //
					//leftHand.release();
					rightHand.release();
					//fourier.release();
				}
				pBody->Release();
				pBody = NULL;
			}
		}
		depthFrame->Release();
		depthFrame = NULL;

		//colorFrame->Release();
		//colorFrame = NULL;

		int t1 = std::time(nullptr);
		if (t1 - t0 > 60);
			//break;
	}
	

	f.close();
	return 0;
}

