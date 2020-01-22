/* barcodereader.cpp - mbreit

	Copyright 2017 - 2019 Matthew Breit <matt.breit@gmail.com>

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.

	THIS SOFTWARE REQUIRES ADDITIONAL SOFTWARE (IE: LIBRARIES) IN ORDER TO COMPILE
	INTO BINARY FORM AND TO FUNCTION IN BINARY FORM. ANY SUCH ADDITIONAL SOFTWARE
	IS OUTSIDE THE SCOPE OF THIS LICENSE.


	BARCODE SOFTWARE LICENSE (zxing)
	Software License Agreement (BSD License)
	----------------------------------------
	Copyright (c) 2013 IDEAL Software GmbH, Neuss, Germany. All rights reserved.
	www.IdealSoftware.com

	Redistribution and use in source and binary forms, with or without modification,
	are permitted provided that the following conditions are met:

	* Redistributions of source code must retain the above copyright notice, this
	list of conditions and the following disclaimer.

	* Redistributions in binary form must reproduce the above copyright notice, this
	list of conditions and the following disclaimer in the documentation and/or
	other materials provided with the distribution.

	* Neither the name IDEAL Software, nor the names of contributors
	may be used to endorse or promote products derived from this software without
	specific prior written permission of IDEAL Software.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
	ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
	*/


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

// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

// Number of images to be grabbed.
static const uint32_t c_countOfImagesToGrab = 1000;

class BarcodeReader
{
private:
	class PylonImageSource : public zxing::LuminanceSource
	{
	private:
		Pylon::CPylonImage m_Image;

	public:
		PylonImageSource(Pylon::CPylonImage &image) : LuminanceSource(image.GetWidth(), image.GetHeight())
		{
			m_Image.CopyImage(image);
		}

		~PylonImageSource()
		{}

		int getWidth() const { return m_Image.GetWidth(); }
		int getHeight() const { return m_Image.GetHeight(); }

		zxing::ArrayRef<char> getRow(int y, zxing::ArrayRef<char> row) const //See Zxing Array.h for ArrayRef def
		{
			int width_ = getWidth();
			if (!row)
				row = zxing::ArrayRef<char>(width_);
			int yoffset = y * width_;
			uint8_t* pBuffer = (uint8_t*)m_Image.GetBuffer();
			for (int x = 0; x < width_; ++x)
			{
				row[x] = pBuffer[yoffset + x];
			}

			return row;
		}

		zxing::ArrayRef<char> getMatrix() const
		{
			int width_ = getWidth();
			int height_ = getHeight();
			zxing::ArrayRef<char> matrix = zxing::ArrayRef<char>(width_ * height_);
			uint8_t* pBuffer = (uint8_t*)m_Image.GetBuffer();
			for (int y = 0; y < height_; ++y)
			{
				int yoffset = y * width_;
				for (int x = 0; x < width_; ++x)
				{
					matrix[yoffset + x] = pBuffer[yoffset + x];
				}
			}
			return matrix;
		}

		CPylonImage GetImage()
		{
			return m_Image;
		}
		/*
		// The following methods are not supported by this demo (the DataMatrix Reader doesn't call these methods)
		bool isCropSupported() const { return false; }
		Ref<LuminanceSource> crop(int left, int top, int width, int height) {}
		bool isRotateSupported() const { return false; }
		Ref<LuminanceSource> rotateCounterClockwise() {}
		*/
	};

	zxing::MultiFormatReader m_reader;

public:
	BarcodeReader(){}
	~BarcodeReader(){}

	struct BRResult
	{
		bool BarcodeFound = false;
		double XLocation = -1;
		double YLocation = -1;
		std::string BarcodeData = "";
		std::string ErrorMessage = "";
	};

	BRResult ReadImage(CPylonImage image)
	{
		BRResult r;

		zxing::Ref<PylonImageSource> source(new PylonImageSource(image));
		zxing::Ref<zxing::Binarizer> binarizer(new zxing::GlobalHistogramBinarizer(source));
		zxing::Ref<zxing::BinaryBitmap> bitmap(new zxing::BinaryBitmap(binarizer));

		try
		{
			zxing::Ref<zxing::Result> result = m_reader.decode(bitmap);
			r.BarcodeFound = true;
			r.BarcodeData = result->getText()->getText();
			zxing::ArrayRef<zxing::Ref<zxing::ResultPoint>> pts = result->getResultPoints();
			r.XLocation = pts[0]->getX();
			r.YLocation = pts[0]->getY();
		}
		catch (zxing::Exception& e)
		{
			r.BarcodeFound = false;
			r.BarcodeData = "";
			r.XLocation = -1;
			r.YLocation = -1;
			r.ErrorMessage = e.what();
		}

		return r;
	}
};

int main(int argc, char* argv[])
{
	// The exit code of the sample application.
	int exitCode = 0;

	// Automagically call PylonInitialize and PylonTerminate to ensure the pylon runtime system
	// is initialized during the lifetime of this object.
	Pylon::PylonAutoInitTerm autoInitTerm;

	try
	{
		// Create an instant camera object with the camera device found first.
		CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());

		// Print the model name of the camera.
		cout << "Using device " << camera.GetDeviceInfo().GetModelName() << endl;

		camera.Open();
		GenApi::CEnumerationPtr(camera.GetNodeMap().GetNode("UserSetSelector"))->FromString("Default");
		GenApi::CCommandPtr(camera.GetNodeMap().GetNode("UserSetLoad"))->Execute();

		if (GenApi::IsWritable(camera.GetNodeMap().GetNode("ExposureAuto")))
			GenApi::CEnumerationPtr(camera.GetNodeMap().GetNode("ExposureAuto"))->FromString("Continuous");
		if (GenApi::IsWritable(camera.GetNodeMap().GetNode("GainAuto")))
			GenApi::CEnumerationPtr(camera.GetNodeMap().GetNode("GainAuto"))->FromString("Continuous");

		if (GenApi::IsWritable(camera.GetNodeMap().GetNode("BslBrightness")))
			GenApi::CFloatPtr(camera.GetNodeMap().GetNode("BslBrightness"))->SetValue(0.4);
		if (GenApi::IsWritable(camera.GetNodeMap().GetNode("BslContrast")))
			GenApi::CFloatPtr(camera.GetNodeMap().GetNode("BslContrast"))->SetValue(GenApi::CFloatPtr(camera.GetNodeMap().GetNode("BslContrast"))->GetMax());
		if (GenApi::IsWritable(camera.GetNodeMap().GetNode("AutoTargetBrightness")))
			GenApi::CFloatPtr(camera.GetNodeMap().GetNode("AutoTargetBrightness"))->SetValue(0.5);

		GenApi::CEnumerationPtr(camera.GetNodeMap().GetNode("TriggerSelector"))->FromString("FrameStart");
		GenApi::CEnumerationPtr(camera.GetNodeMap().GetNode("TriggerSource"))->FromString("Software");
		GenApi::CEnumerationPtr(camera.GetNodeMap().GetNode("TriggerMode"))->FromString("On");

		int64_t width = GenApi::CIntegerPtr(camera.GetNodeMap().GetNode("Width"))->GetValue();
		int64_t height = GenApi::CIntegerPtr(camera.GetNodeMap().GetNode("Height"))->GetValue();

		BarcodeReader myBarcodeReader;

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
				cout << endl;
				cout << "Image Received: " << ptrGrabResult->GetBlockID() << endl;

				// use pylon to convert whatever format the incoming image is to Mono8
				Pylon::CImageFormatConverter fc;
				fc.OutputPixelFormat = Pylon::PixelType_Mono8;
				Pylon::CPylonImage pylonImage;
				if (fc.ImageHasDestinationFormat(ptrGrabResult))
					pylonImage.AttachGrabResultBuffer(ptrGrabResult);
				else
					fc.Convert(pylonImage, ptrGrabResult);
#ifdef PYLON_WIN_BUILD
				Pylon::DisplayImage(0, pylonImage);
#endif
				// Todo for Linux:
				// We can bring back some OpenCV code here to display the image since Pylon::DisplayImage does not support Linux.
				
				BarcodeReader myBarcodeReader;
				BarcodeReader::BRResult myResult;

				myResult = myBarcodeReader.ReadImage(pylonImage);

				if (myResult.BarcodeFound == true)
				{
					cout << "Barcode Found: " << endl;
					cout << " Location : " << "X: " << myResult.XLocation << " Y: " << myResult.YLocation << endl;
					cout << " Data     : " << myResult.BarcodeData << endl;
				}
				else
					cout << "Barcode Not Found: " << myResult.ErrorMessage << endl;
			}
			else
			{
				cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
			}
		}
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
