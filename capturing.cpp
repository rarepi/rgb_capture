#include <cstdio>
#include <iostream>
#include <math.h>
#include <list>
#include <chrono>
#include <thread>
#include <sstream>
#include <vector>
#include <tuple>
#include <windows.h>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <fstream>

using namespace std::chrono;

const int LOOP_INTERVAL = 100; //ms
const int BORDER_SIZE = 100; //size of area to read for color data
const int CHUNK_SIZE = 63; //size of area chunks per LED

int CaptureAnImage();

struct CaptureRegion {
	int border;
	int x;		//x
	int y;		//y
	int width;	//width
	int height;	//height
};

struct RAWRGB {
	int R;
	int G;
	int B;

	RAWRGB operator+(const RAWRGB& other) {
		this->R += other.R;
		this->G += other.G;
		this->B += other.B;
		return *this;
	}

	RAWRGB operator/(const int& other) {
		this->R /= other;
		this->G /= other;
		this->B /= other;
		return *this;
	}

	std::string toString() {
		return std::to_string(R) + "," + std::to_string(G) + "," + std::to_string(B);
	}
};

int main() {
	CaptureAnImage();
	std::cin.get();
	return 0;
}

bool compare_CaptureRegion(const CaptureRegion first, const CaptureRegion second)
{
	return (first.border < second.border);
}

int sendRGB(RAWRGB* values, int size) {
	for (int i = 0; i < size; i++) {
		values[i];
	}
	return 0;
}

int CaptureAnImage()
{
	HWND hWnd = GetDesktopWindow();
	HDC hdcScreen;
	HDC hdcWindow;
	HDC hdcMemDC = NULL;
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;

	// Retrieve the handle to a display device context for the client area of the window. 
	hdcScreen = GetDC(NULL);
	hdcWindow = GetDC(hWnd);

	// Create a compatible DC which is used in a BitBlt from the window DC
	hdcMemDC = CreateCompatibleDC(hdcWindow);

	if (!hdcMemDC) {
		MessageBox(hWnd, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
		return -1;
	}

	// Get the client area for size calculation
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	// Create a compatible bitmap from the Window DC
	hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

	if (!hbmScreen) {
		MessageBox(hWnd, L"CreateCompatibleBitmap Failed", L"Failed", MB_OK);
		return -1;
	}

	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC, hbmScreen);

	// Bit block transfer into our compatible memory DC.
	if (!BitBlt(hdcMemDC, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, hdcScreen, 0, 0, SRCCOPY)) {
		MessageBox(hWnd, L"BitBlt has failed", L"Failed", MB_OK);
		return -1;
	}

	// Get the BITMAP from the HBITMAP
	GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

	BITMAPFILEHEADER   bmfHeader;
	//BITMAPINFO bmi;
	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmpScreen.bmWidth;
	bi.biHeight = -bmpScreen.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;
	//bmi.bmiHeader = bi;

	DWORD dwBmpSize = ((bi.biWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	unsigned char *lpbitmap = (unsigned char *)GlobalLock(hDIB);

	// Gets the "bits" from the bitmap and copies them into a buffer which is pointed to by lpbitmap.
	GetDIBits(hdcWindow, hbmScreen, 0, (UINT)bmpScreen.bmHeight, lpbitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	RGBQUAD *pPixels = new RGBQUAD[bmpScreen.bmWidth * bmpScreen.bmHeight];

	std::list<CaptureRegion> captureRegionList;
	for (int x = 0, i = 0; x < bmpScreen.bmWidth; x = x + CHUNK_SIZE, i++) {
		if (x > bmpScreen.bmWidth - CHUNK_SIZE) {
			x = bmpScreen.bmWidth - CHUNK_SIZE;
		}
		//top area
		CaptureRegion crt;
		crt.border = 0;
		crt.x = x;
		crt.y = 0;
		crt.width = CHUNK_SIZE;
		crt.height = BORDER_SIZE;
		////bottom area
		CaptureRegion crb;
		crb.border = 1;
		crb.x = x;
		crb.y = bmpScreen.bmHeight - BORDER_SIZE;
		crb.width = CHUNK_SIZE;
		crb.height = BORDER_SIZE;

		captureRegionList.push_back(crt);
		captureRegionList.push_back(crb);
	}
	for (int y = 0, i = 0; y < bmpScreen.bmHeight; y = y + CHUNK_SIZE, i++) {
		if (y > bmpScreen.bmHeight - CHUNK_SIZE) {
			y = bmpScreen.bmHeight - CHUNK_SIZE;
		}
		//left area
		CaptureRegion crl;
		crl.border = 2;
		crl.x = 0;
		crl.y = y;
		crl.width = BORDER_SIZE;
		crl.height = CHUNK_SIZE;
		////right area
		CaptureRegion crr;
		crr.border = 3;
		crr.x = bmpScreen.bmWidth - BORDER_SIZE;
		crr.y = y;
		crr.width = BORDER_SIZE;
		crr.height = CHUNK_SIZE;

		captureRegionList.push_back(crl);
		captureRegionList.push_back(crr);
	}
	captureRegionList.sort(compare_CaptureRegion);

	int captureRegionArraySize = captureRegionList.size();
	CaptureRegion *captureRegionArray = new CaptureRegion[captureRegionArraySize];
	int i = 0;
	for (CaptureRegion captureRegion : captureRegionList) {
		captureRegionArray[i] = captureRegion;
		i++;
	}

	std::ofstream myfile;
	myfile.open("data.txt");
	auto start_total = high_resolution_clock::now();
	for (int loops = 0;loops < 5;loops++) {
		auto start = high_resolution_clock::now();
		BitBlt(hdcMemDC, 0, 0, bmpScreen.bmWidth, bmpScreen.bmHeight, hdcScreen, 0, 0, SRCCOPY);
		std::cout << "LOOP " << loops << " ~ Grabbing image in " << duration<double, std::milli>(high_resolution_clock::now() - start).count() << " milliseconds.\n";
		
		auto looptime = high_resolution_clock::now();
		RAWRGB* values = new RAWRGB[captureRegionArraySize];
		for (int i = 0; i < captureRegionArraySize; i++) {
			RAWRGB value = { 0,0,0 };
			for (int y = captureRegionArray[i].y; y < captureRegionArray[i].y + captureRegionArray[i].height; y++) {
				for (int x = captureRegionArray[i].x; x < captureRegionArray[i].x + captureRegionArray[i].width; x++) {
					RAWRGB rgb = { lpbitmap[4 * ((y*bmpScreen.bmWidth) + x) + 2], lpbitmap[4 * ((y*bmpScreen.bmWidth) + x) + 1], lpbitmap[4 * ((y*bmpScreen.bmWidth) + x)] };
					value = rgb + value;
				}
			}
			values[i] = value / (captureRegionArray[i].height * captureRegionArray[i].width);
		}
		std::cout << "LOOP " << loops << " ~ RGB fetch in " << duration<double, std::milli>(high_resolution_clock::now() - looptime).count() << " milliseconds.\n";
		/*for (int i = 0; i < captureRegionArraySize; i++) {
			myfile << values[i].toString() << "\n";
		}*/
		//sendRGB(values, captureRegionArraySize);
		std::this_thread::sleep_for(milliseconds(LOOP_INTERVAL)-duration<double, std::milli>(high_resolution_clock::now() - start));
		std::cout << "LOOP " << loops << " ~ Full Loop in " << duration<double, std::milli>(high_resolution_clock::now() - start).count() << " milliseconds.\n";
		std::cout << "Loops took " << duration<double, std::milli>(high_resolution_clock::now() - start_total).count() << " milliseconds so far.\n";
	}
	std::cout << "All loops in " << duration<double, std::milli>(high_resolution_clock::now() - start_total).count() << " milliseconds.\n";
	myfile.close();

	// A file is created, this is where we will save the screen capture.
	HANDLE hFile = CreateFile(
		L"captureqwsx.bmp",
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	//Offset to where the actual bitmap bits start.
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
	//Size of the file
	bmfHeader.bfSize = dwSizeofDIB;
	//bfType must always be BM for Bitmaps
	bmfHeader.bfType = 0x4D42; //BM   

	DWORD dwBytesWritten = 0;
	WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

	//Unlock and Free the DIB from the heap
	GlobalUnlock(hDIB);
	GlobalFree(hDIB);

	CloseHandle(hFile);

	//Clean up
	DeleteObject(hbmScreen);
	DeleteObject(hdcMemDC);
	ReleaseDC(NULL, hdcScreen);
	ReleaseDC(hWnd, hdcWindow);

	std::cout << "done";
	return 0;
}