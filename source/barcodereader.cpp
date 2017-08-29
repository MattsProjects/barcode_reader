// PylonSample_Barcode.cpp - mbreit
/*
Note: Before getting started, Basler recommends reading the Programmer's Guide topic
in the pylon C++ API documentation that gets installed with pylon.
If you are upgrading to a higher major version of pylon, Basler also
strongly recommends reading the Migration topic in the pylon C++ API documentation.

This sample illustrates how to grab and process images using the CInstantCamera class.
The images are grabbed and processed asynchronously, i.e.,
while the application is processing a buffer, the acquisition of the next buffer is done
in parallel.

The CInstantCamera class uses a pool of buffers to retrieve image data
from the camera device. Once a buffer is filled and ready,
the buffer can be retrieved from the camera object for processing. The buffer
and additional image data are collected in a grab result. The grab result is
held by a smart pointer after retrieval. The buffer is automatically reused
when explicitly released or when the smart pointer object is destroyed.
*/
// BARCODE SOFTWARE LICENSE
//	Software License Agreement (BSD License)
//	----------------------------------------
//	Copyright (c) 2013 IDEAL Software GmbH, Neuss, Germany. All rights reserved.
//                              www.IdealSoftware.com
//
//	Redistribution and use in source and binary forms, with or without modification,
//	are permitted provided that the following conditions are met:
//
//	* Redistributions of source code must retain the above copyright notice, this 
//	  list of conditions and the following disclaimer.
//
//	* Redistributions in binary form must reproduce the above copyright notice, this
//	  list of conditions and the following disclaimer in the documentation and/or
//	  other materials provided with the distribution.
//
//	* Neither the name IDEAL Software, nor the names of contributors
//	  may be used to endorse or promote products derived from this software without
//	  specific prior written permission of IDEAL Software.
//
//	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
//	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//	ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//


#include <opencv/cv.h> 
#include <opencv/highgui.h> 

#include <stdio.h>
#include <iostream>

#include "zxing/LuminanceSource.h"
#include "zxing/MultiFormatReader.h"
#include "zxing/oned/OneDReader.h"
#include "zxing/oned/EAN8Reader.h"
#include "zxing/oned/EAN13Reader.h"
#include "zxing/oned/Code128Reader.h"
#include "zxing/datamatrix/DataMatrixReader.h"
#include "zxing/qrcode/QRCodeReader.h"
#include "zxing/aztec/AztecReader.h"
#include "zxing/common/GlobalHistogramBinarizer.h"
#include "zxing/Exception.h"

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif

#define USE_USB

#if defined( USE_1394 )
// Settings for using Basler IEEE 1394 cameras.
#include <pylon/1394/Basler1394InstantCamera.h>
typedef Pylon::CBasler1394InstantCamera Camera_t;
using namespace Basler_IIDC1394CameraParams;
#elif defined ( USE_GIGE )
// Settings for using Basler GigE cameras.
#include <pylon/gige/BaslerGigEInstantCamera.h>
typedef Pylon::CBaslerGigEInstantCamera Camera_t;
using namespace Basler_GigECameraParams;
#elif defined ( USE_CAMERALINK )
// Settings for using Basler Camera Link cameras.
#include <pylon/cameralink/BaslerCameraLinkInstantCamera.h>
typedef Pylon::CBaslerCameraLinkInstantCamera Camera_t;
using namespace Basler_CLCameraParams;
#elif defined ( USE_USB )
// Settings for using Basler USB cameras.
#include <pylon/usb/BaslerUsbInstantCamera.h>
typedef Pylon::CBaslerUsbInstantCamera Camera_t;
using namespace Basler_UsbCameraParams;
#else
#error Camera type is not specified. For example, define USE_GIGE for using GigE cameras.
#endif

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

using namespace zxing;
using namespace oned;
using namespace datamatrix;
using namespace qrcode;
using namespace aztec;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 100;

class OpenCVBitmapSource : public LuminanceSource
{
private:
	cv::Mat m_pImage;

public:
	OpenCVBitmapSource(cv::Mat &image)
		: LuminanceSource(image.cols, image.rows)
	{
		m_pImage = image.clone();
	}

	~OpenCVBitmapSource(){}

	int getWidth() const { return m_pImage.cols; }
	int getHeight() const { return m_pImage.rows; }

	ArrayRef<char> getRow(int y, ArrayRef<char> row) const //See Zxing Array.h for ArrayRef def
	{
		int width_ = getWidth();
		if (!row)
			row = ArrayRef<char>(width_);
		const char *p = m_pImage.ptr<char>(y);
		for (int x = 0; x<width_; ++x, ++p)
			row[x] = *p;
		return row;
	}

	ArrayRef<char> getMatrix() const
	{
		int width_ = getWidth();
		int height_ = getHeight();
		ArrayRef<char> matrix = ArrayRef<char>(width_*height_);
		for (int y = 0; y < height_; ++y)
		{
			const char *p = m_pImage.ptr<char>(y);
			int yoffset = y*width_;
			for (int x = 0; x < width_; ++x, ++p)
			{
				matrix[yoffset + x] = *p;
			}
		}
		return matrix;
	}
	/*
	// The following methods are not supported by this demo (the DataMatrix Reader doesn't call these methods)
	bool isCropSupported() const { return false; }
	Ref<LuminanceSource> crop(int left, int top, int width, int height) {}
	bool isRotateSupported() const { return false; }
	Ref<LuminanceSource> rotateCounterClockwise() {}
	*/
};

void decode_image(Reader *reader, IplImage *image)
{
	try
	{
		cv::Mat m = cv::cvarrToMat(image);
		Ref<OpenCVBitmapSource> source(new OpenCVBitmapSource(m));
		Ref<Binarizer> binarizer(new GlobalHistogramBinarizer(source));
		Ref<BinaryBitmap> bitmap(new BinaryBitmap(binarizer));
		Ref<Result> result(reader->decode(bitmap, DecodeHints(DecodeHints::DEFAULT_HINT)));
		cout << "Barcode value is: " << result->getText() << endl;
		//Beep(440, 100);
	}
	catch (zxing::Exception& e)
	{
		 cerr << "Error: " << e.what() << endl;
	}
}

int main(int argc, char* argv[])
{
	// The exit code of the sample application.
	int exitCode = 0;

	// Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
	// is initialized during the lifetime of this object.
	Pylon::PylonAutoInitTerm autoInitTerm;

	try
	{
			
		CDeviceInfo info;
		String_t serialNumber = "";
		cout << "Enter Serial Number (dart only!): ";
		cin >> serialNumber;
		info.SetSerialNumber(serialNumber);

		// Create an instant camera object with the camera device found first.
		Camera_t camera(CTlFactory::GetInstance().CreateFirstDevice(info));

		// Print the model name of the camera.
		cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

		camera.Open();
		camera.UserSetSelector.SetValue(UserSetSelector_Default);
		camera.UserSetLoad.Execute();

		camera.PixelFormat.SetValue(PixelFormat_Mono8);
		camera.ExposureAuto.SetValue(ExposureAuto_Continuous);
		camera.BslBrightness.SetValue(0.4);
		camera.BslContrast.SetValue(0.4);

		cout << "Grabbing 100 images to settle autoexposure..." << endl;
		camera.StartGrabbing(100);
		while (camera.IsGrabbing())
		{
			CGrabResultPtr result;
			camera.RetrieveResult(1000, result, TimeoutHandling_ThrowException);
			cout << ".";
		}

		cout << endl << "Starting Barcode Reading..." << endl;
		camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
		camera.TriggerSource.SetValue(TriggerSource_Software);
		camera.TriggerMode.SetValue(TriggerMode_On);

		int width, height;
		width = camera.Width.GetValue();
		height = camera.Height.GetValue();	

		IplImage *frame;		

		MultiFormatReader reader;

		IplImage *gray = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
		if (!gray)
		{
			fprintf(stderr, "ERROR: allocating image\n");
			getchar();
			return -1;
		}

		// create crop image
		int target_width = width * 2 / 3;
		int target_height = height * 2 / 3;
		int startx = (width - target_width) / 2;
		int starty = (height - target_height) / 2;
		IplImage *crop = cvCreateImage(cvSize(target_width, target_height), IPL_DEPTH_8U, 1);
		if (!crop)
		{
			fprintf(stderr, "ERROR: allocating image\n");
			getchar();
			return -1;
		}

		// Create a window in which the captured images will be presented
		cvNamedWindow("Barcode Reader", CV_WINDOW_AUTOSIZE);

		// The parameter MaxNumBuffer can be used to control the count of buffers
		// allocated for grabbing. The default value of this parameter is 10.
		camera.MaxNumBuffer = 5;

		// Start the grabbing of c_countOfImagesToGrab images.
		// The camera device is parameterized with a default configuration which
		// sets up free-running continuous acquisition.
		camera.StartGrabbing(c_countOfImagesToGrab);

		// This smart pointer will receive the grab result data.
		CGrabResultPtr ptrGrabResult;

		// Camera.StopGrabbing() is called automatically by the RetrieveResult() method
		// when c_countOfImagesToGrab images have been retrieved.
		while (camera.IsGrabbing())
		{
			camera.ExecuteSoftwareTrigger();

			// Wait for an image and then retrieve it. A timeout of 5000 ms is used.
			camera.RetrieveResult(5000, ptrGrabResult, TimeoutHandling_ThrowException);

			// Image grabbed successfully?
			if (ptrGrabResult->GrabSucceeded())
			{
				cout << "Image Received: " << ptrGrabResult->GetBlockID() << endl;

				Pylon::CImageFormatConverter fc; // for converting the image to/from different formats
				fc.OutputPixelFormat = Pylon::PixelType_BGR8packed; // OpenCV uses BGR8packed format
				Pylon::CPylonImage pylonImage; // the original image container for the grab result.
				// convert the image to BGR8packed
				fc.Convert(pylonImage, ptrGrabResult);
				cv::Mat cv_img;
				cv_img = cv::Mat(ptrGrabResult->GetHeight(), ptrGrabResult->GetWidth(), CV_8UC3, (uint8_t*)pylonImage.GetBuffer());
				frame = new IplImage(cv_img);

				// CV_RGB2GRAY: convert RGB image to grayscale
				cvCvtColor(frame, gray, CV_RGB2GRAY);

				// crop image
				cvSetImageROI(gray, cvRect(startx, starty, target_width, target_height));
				cvCopy(gray, crop, NULL);
				cvResetImageROI(gray);

				// display resized original and paint crop rectangle
				float scale = 0.8f;
				IplImage *resized = cvCreateImage(cvSize((int)(width * scale), (int)(height * scale)), gray->depth, gray->nChannels);
				cvResize(gray, resized);

				// paint target crop rectangle
				int sstartx = (int)(startx * scale);
				int starget_width = (int)(target_width * scale);
				int sstarty = (int)(starty * scale);
				int starget_height = (int)(target_height * scale);
				for (int x = sstartx; x < sstartx + starget_width; x++)
				{
					resized->imageData[sstarty * resized->widthStep + x] = 0;
					resized->imageData[(sstarty + starget_height) * resized->widthStep + x] = 0;
				}

				for (int y = sstarty; y < sstarty + starget_height; y++)
				{
					resized->imageData[y * resized->widthStep + sstartx] = 0;
					resized->imageData[y * resized->widthStep + sstartx + starget_width] = 0;
				}

				cvShowImage("Barcode Reader", resized);
				cvWaitKey(1);
				cvReleaseImage(&resized);

				// decode barcode
				decode_image(&reader, crop);

			}
			else
			{
				cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
			}
		}

		cvDestroyWindow("Barcode Reader");
		cvReleaseImage(&gray);
		cvReleaseImage(&crop);

		camera.Close();
	}
	catch (GenICam::GenericException &e)
	{
		// Error handling.
		cerr << "An exception occurred." << endl
			<< e.GetDescription() << endl;
		exitCode = 1;
	}

	// Comment the following two lines to disable waiting on exit.
	cerr << endl << "Press Enter to exit." << endl;
	while (cin.get() != '\n');

	return exitCode;
}
