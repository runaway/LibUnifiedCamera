#pragma once

extern "C" _declspec(dllexport) bool VideoCameraOpen(unsigned int& nVideoidth, unsigned int& nVideoWidth);
extern "C" _declspec(dllexport) bool VideoCameraGrabFrame(unsigned char* pbyImageData);