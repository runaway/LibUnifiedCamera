#include "stdafx.h"
#include "DefectDetection.h"
#include <vector>
#include <shlwapi.h>
#include "ComFunc.h"
#include "DefectDetectionGPU.h"
#include "DefectDetectionNoRotation.h"
#include "ParallelDiff.h"

#include <windows.h>


#pragma comment(lib, "shlwapi.lib")

// 目标机是否有CUDA
extern bool gHaveCUDA;

// 是否已检测过目标机CUDA的设置
extern bool gHaveQueryCUDA;


using namespace cv;
using namespace std;

Mat matDebug(Size(640, 480), CV_8UC3, Scalar(0));

// ------------------------------------------------------------
// Description : 缺陷检测
// Parameters :  dstImg: 待检测图片
//				 goldenImg: 金标准图片
//				 defectRegions: 待检测图片与金标准图片相减后的
//				 图片
//				 bWithRotate: 是否需要旋转匹配
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectDetection::Detection(
	cv::Mat& dstImg,
	const Mat& goldenImg, 
	const Mat& maskImg,
	cv::Rect siteRect,
	unsigned char thresholdVal,
	cv::Mat& diffMatFg,
	cv::Mat& diffMatBg,
	cv::Mat& diffMat,
	bool bWithRotate,
	MatchType matchtype)
{	
	int iShiftX;
	int iShiftY;
	
	if (dstImg.empty())
	{
		cout << "DefectDetection::Detection() error: dstImg.empty()" << endl;
		return false;
	}
	
	if (siteRect.width <= 0 || siteRect.height <= 0)
	{
		siteRect.x = 0;
		siteRect.y = 0;
		siteRect.width = goldenImg.cols;
		siteRect.height = goldenImg.rows;
	}
	else if (siteRect.width > goldenImg.cols || siteRect.height > goldenImg.rows)
	{
		siteRect.width = goldenImg.cols;
		siteRect.height = goldenImg.rows;
	}

	//Mat diffMatFg, diffMatBg;
#ifdef WITH_CUDA
	if (gHaveCUDA)
	{
		vector<Mat> vecDstMat;
		vecDstMat.push_back(dstImg);
		vector<Mat> vecDiffMat;

		if (bWithRotate)
		{
			DefectDetectionGPU::GetInstance()->image_abs(goldenImg, siteRect, vecDstMat, vecDiffMat);
		}
		else
		{
			DefectDetectionNoRotation::GetInstance()->image_abs(goldenImg, siteRect, vecDstMat, vecDiffMat);
		}

		diffMatFg = vecDiffMat[0];
	} 
	else
#endif
	{
		if (!GetDiffRegions(dstImg, siteRect, goldenImg, maskImg, diffMatFg, diffMatBg, diffMat, iShiftX, iShiftY, matchtype))
		{
			return false;
		}
		//printf("match shift: x=%d, y=%d \n", iShiftX, iShiftY);
	}
	
/*	threshold(diffMatFg, defectRegionsFg, thresholdVal, 255, THRESH_BINARY);

	Mat m0;
	threshold(diffMatBg, defectRegionsBg, 80, 255, THRESH_BINARY);
	threshold(dstImg, m0, 180, 255, THRESH_BINARY);		//180是创建mask的阈值
	defectRegionsBg = defectRegionsBg & m0; */
	return true;
}

// ------------------------------------------------------------
// Description : 缺陷检测(批量处理)
// Parameters :  dstImgPath: 待检测图片文件夹路径
//				 goldenImgPath: 金标准图片文件夹路径
//				 thresholdVal: 灰度阈值
//				 siteRect: site的检测区域	
//				 defectRegionsVec: 待检测图片与金标准图片相减后的
//				 图片
//				 bParallel: cpu版本是否使用并行计算
//				 bWithRotate: 是否需要旋转匹配
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectDetection::Detection(
	vector<string>& imgNameVec,
	const std::string& dstImgPath,
	const Mat& matGoldenImg,
	unsigned char thresholdVal,
	cv::Rect siteRect,
	std::vector<cv::Mat>& defectRegionsVec,
	bool bParallel,
	bool bWithRotate)
{		
	if (imgNameVec.empty())
	{
		cout << "DefectDetection::Detection() error: imgNameVec.empty()" << endl;
		return false;
	}

	long startTime = GetTickCount();

	if (siteRect.width <= 0 || siteRect.height <= 0)
	{
		siteRect.x = 0;
		siteRect.y = 0;
		siteRect.width = matGoldenImg.cols;
		siteRect.height = matGoldenImg.rows;
	}
	else if (siteRect.width > matGoldenImg.cols || siteRect.height > matGoldenImg.rows)
	{
		siteRect.width = matGoldenImg.cols;
		siteRect.height = matGoldenImg.rows;
	}

	long endTime = GetTickCount();
	cout << "     ==== create golden image cost time : " << endTime - startTime << " ms" << endl;
	startTime = endTime;

	vector<Mat> vecDstMat;
	
	vector<string>::iterator imgNameIt = imgNameVec.begin();

    for (; imgNameIt != imgNameVec.end(); ++imgNameIt)
	{		
		string fname = dstImgPath + *imgNameIt;
		if (PathFileExists(fname.c_str())
			&& strcmp(imgNameIt->c_str(), ".") != 0
			&& strcmp(imgNameIt->c_str(), "..") != 0)
		{
			vecDstMat.push_back(imread(fname.c_str(), CV_LOAD_IMAGE_GRAYSCALE));
		}			
	}	

	endTime = GetTickCount();
	cout << "     ==== insert images to vector cost time : " << endTime - startTime << " ms" << endl;
	startTime = endTime;

	defectRegionsVec.clear();
#ifdef WITH_CUDA    
	if (gHaveCUDA)
	{
		vector<Mat> vecDiffMat;

		if (bWithRotate)
		{
			DefectDetectionGPU::GetInstance()->image_abs(matGoldenImg, siteRect, vecDstMat, vecDiffMat);
		}
		else
		{
			DefectDetectionNoRotation::GetInstance()->image_abs(matGoldenImg, siteRect, vecDstMat, vecDiffMat);
		}

		vector<Mat>::iterator diffMatIt = vecDiffMat.begin();

        for (; diffMatIt != vecDiffMat.end(); ++diffMatIt)
		{
			Mat tmpRegionMat(diffMatIt->size(), CV_8U);
			threshold(*diffMatIt, tmpRegionMat, thresholdVal, 255, THRESH_BINARY);
			defectRegionsVec.push_back(tmpRegionMat);
		}
	} 
	else
#endif        
	{
		vector<Mat> vecDiffMat(vecDstMat.size());
        
		if (bParallel)
		{
			int *shiftxlist = new int[vecDstMat.size()];
			int *shiftylist = new int[vecDstMat.size()];

			ParallelDiff imagediff(&vecDiffMat, shiftxlist, shiftylist, 5, // 2 
				&vecDstMat,
				matGoldenImg, siteRect, matGoldenImg.size(), matGoldenImg.type());
			parallel_for_(Range(0, vecDstMat.size()), imagediff);

			delete[] shiftxlist;
			delete[] shiftylist;
		}
		else if (!GetBatchDiffRegions(vecDstMat, siteRect, matGoldenImg, vecDiffMat))
		{
			return false;
		}		

		vector<Mat>::iterator diffMatIt = vecDiffMat.begin();
        
		for (; diffMatIt != vecDiffMat.end(); ++diffMatIt)
		{
			Mat tmpRegionMat(diffMatIt->size(), CV_8U);
			threshold(*diffMatIt, tmpRegionMat, thresholdVal, 255, THRESH_BINARY);
			defectRegionsVec.push_back(tmpRegionMat);
		}
	}
	
	return true;
}

// ------------------------------------------------------------
// Description : 创建golden image
// Parameters :  goldenImgPath: 创建金标准图片的路径
//				 goldenImg: 金标准图片
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectDetection::CreateGoldenImage(
	const std::string& goldenImgPath,
	cv::Mat& goldenImg)
{
	if (!PathIsDirectory(goldenImgPath.c_str()))
	{
		return false;
	}

	Mat src1, src2, dst;	
	vector<Mat> images;
	Mat a, b, c, d, e, f, g, h, im1, im2, im3;	

	vector<string> imgFileNameVec;
	GetImgNameFromDir(goldenImgPath, imgFileNameVec);
	vector<string>::iterator imgFileNameVecIt = imgFileNameVec.begin();
	
	for (int nCount = 0; 
         imgFileNameVecIt != imgFileNameVec.end() && nCount < 10;
		 ++imgFileNameVecIt)
	{
		string fname = goldenImgPath + *imgFileNameVecIt;
        
		if (PathFileExists(fname.c_str())
			&& strcmp(imgFileNameVecIt->c_str(), ".") != 0
			&& strcmp(imgFileNameVecIt->c_str(), "..") != 0)
		{
			images.push_back(imread(fname.c_str(), CV_LOAD_IMAGE_GRAYSCALE));
			++nCount;
		}
	}

	if (images.size() < 9)
	{
		cout << "DefectDetection::CreateGoldenImage() images.size() < 9" << endl;
		return false;
	}

	// get the median of 9 images below
	a = images[0]; b = images[1]; c = images[2];
	cv::max(a, b, d);
	cv::max(b, c, e);
	cv::max(a, c, f);
	cv::min(e, f, g);
	cv::min(d, e, h);
	cv::min(g, h, im1);

	a = images[3]; b = images[4]; c = images[5];
	cv::max(a, b, d);
	cv::max(b, c, e);
	cv::max(a, c, f);
	cv::min(e, f, g);
	cv::min(d, e, h);
	cv::min(g, h, im2);

	a = images[6]; b = images[7]; c = images[8];
	cv::max(a, b, d);
	cv::max(b, c, e);
	cv::max(a, c, f);
	cv::min(e, f, g);
	cv::min(d, e, h);
	cv::min(g, h, im3);

	a = im1; b = im2; c = im3;
	max(a, b, d);
	max(b, c, e);
	max(a, c, f);
	min(e, f, g);
	min(d, e, h);
	min(g, h, goldenImg);		
	
	return true;
}
// ------------------------------------------------------------
// Description : 创建golden image
// Parameters :  goldenImg: 金标准图片
//				 goldenMaskImg:  金标准mask图片，即金标准图片的有效像素图片
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectDetection::CreateGoldenMaskImage(
	const Mat& goldenImg,
	Mat& goldenMaskImg)
{
	if (goldenImg.empty())
	{
		cout << "DefectDetection::CreateGoldenMaskImage() error: goldenImg.empty()" << endl;
		return false;
	}


	Mat mo1, mo2;
	threshold(goldenImg, mo1, 220, 255, THRESH_BINARY);
	Mat element0 = getStructuringElement(MORPH_RECT, Size(5, 5), Point(-1, -1));
	Mat element1 = getStructuringElement(MORPH_ELLIPSE, Size(9, 9), Point(-1, -1) );
	Mat element2 = getStructuringElement(MORPH_RECT, Size(9, 9), Point(-1, -1) );
	morphologyEx(mo1, mo1, MORPH_OPEN, element0);
	morphologyEx(mo1, mo2, MORPH_CLOSE, element1);
	erode(mo2, goldenMaskImg, element2);

// 	morphologyEx(mo1, mo2, MORPH_OPEN, element1);
// 	//morphologyEx( mo2, goldenMaskImg, MORPH_OPEN, element2 );
// 	erode(mo2, goldenMaskImg, element2);


#if 0		//第一种 2X
	threshold(M1, mo1, 20, 255, THRESH_BINARY);

	Mat element = getStructuringElement(MORPH_ELLIPSE, Size(3, 3), Point(-1, -1) );
	morphologyEx(mo1, goldenMaskImg, MORPH_OPEN, element);
	//dilate(mo2, maskImg, element);
//#else
	//  第二种 2X
	threshold(M1, mo1, 20+diff, 255, THRESH_BINARY);
	Mat element = getStructuringElement(MORPH_ELLIPSE, Size(3, 3), Point(-1, -1) );
	//morphologyEx(mo1, goldenMaskImg, MORPH_OPEN, element);
	dilate( mo1, goldenMaskImg, element );
#endif
	
	return true;
}

bool DefectDetection::CreateGoldenImage(
	const std::string& strGoldenImgPath,
	cv::Mat& matGoldenImg,
	cv::Mat matBaseImg,
	AlignmentRange sAlignmentRange)
{
	if (!PathIsDirectory(strGoldenImgPath.c_str()))
	{
	    //AfxMessageBox("!PathIsDirectory!");
		return false;
	}

	Mat src1, src2, dst;	
	vector<Mat> vecmatImages;
	Mat a, b, c, d, e, f, g, h, im1, im2, im3;	

	vector<string> vecstrImgFileName;
	GetImgNameFromDir(strGoldenImgPath, vecstrImgFileName);
	vector<string>::iterator itstrImgFileName = vecstrImgFileName.begin();
	
	for (int nCount = 0; 
         itstrImgFileName != vecstrImgFileName.end() && nCount < 10;
		 ++itstrImgFileName)
	{
		string fname = strGoldenImgPath + *itstrImgFileName;
        
		if (PathFileExists(fname.c_str())
			&& strcmp(itstrImgFileName->c_str(), ".") != 0
			&& strcmp(itstrImgFileName->c_str(), "..") != 0)
		{
			vecmatImages.push_back(imread(fname.c_str(), CV_LOAD_IMAGE_GRAYSCALE));
			++nCount;
		}
	}

	if (vecmatImages.size() < 9)
	{
		cout << "DefectDetection::CreateGoldenImage() images.size() < 9" << endl;
        //AfxMessageBox("vecmatImages.size() < 9!");
		return false;
	}

	if(matBaseImg.empty())
	{
		int id = vecmatImages.size() - 1;
		vecmatImages[id].copyTo(matBaseImg);
	}

#if 1
	
    cv::Rect rectSite;
	rectSite.x = 0;
	rectSite.y = 0;
	rectSite.width = matBaseImg.cols;
	rectSite.height = matBaseImg.rows;

    vector<Mat> vecmatDiff(vecmatImages.size());

    //imshow("matBaseImg", matBaseImg);
    //if (!GetBatchDiffRegions(vecmatImages, rectSite, matGoldenImg, vecmatDiff))
  /*  if (!MakeGoldenSite(vecmatImages, rectSite, matBaseImg, matGoldenImg))
	{
	    //AfxMessageBox("!MakeGoldenSite!");
		return false;
	}*/

	if (!CreateGoldenSite(vecmatImages, rectSite, matBaseImg, matGoldenImg, sAlignmentRange))
	{
	    //AfxMessageBox("!MakeGoldenSite!");
		return false;
	}

    
#else    

	// get the median of 9 images below
	a = vecmatImages[0]; b = vecmatImages[1]; c = vecmatImages[2];
	cv::max(a, b, d);
	cv::max(b, c, e);
	cv::max(a, c, f);
	cv::min(e, f, g);
	cv::min(d, e, h);
	cv::min(g, h, im1);

	a = vecmatImages[3]; b = vecmatImages[4]; c = vecmatImages[5];
	cv::max(a, b, d);
	cv::max(b, c, e);
	cv::max(a, c, f);
	cv::min(e, f, g);
	cv::min(d, e, h);
	cv::min(g, h, im2);

	a = vecmatImages[6]; b = vecmatImages[7]; c = vecmatImages[8];
	cv::max(a, b, d);
	cv::max(b, c, e);
	cv::max(a, c, f);
	cv::min(e, f, g);
	cv::min(d, e, h);
	cv::min(g, h, im3);

	a = im1; b = im2; c = im3;
	max(a, b, d);
	max(b, c, e);
	max(a, c, f);
	min(e, f, g);
	min(d, e, h);
	min(g, h, matGoldenImg);		
#endif	
	return true;
}

bool DefectDetection::CreateGoldenSite(
	vector<Mat>& vecmatDstImg,
	Rect rectSite,
	Mat matBaseGoldenImg,
	Mat& matGoldenImg,
	AlignmentRange sAlignmentRange)
{
	if (vecmatDstImg.empty() || matGoldenImg.empty())
	{
	    //AfxMessageBox("vecmatDstImg.empty() || matGoldenImg.empty()!");
		return false;
	}

	if (rectSite.width <= 0 || rectSite.height <= 0)
	{
		rectSite.x = 0;
		rectSite.y = 0;
		rectSite.width = matGoldenImg.cols;
		rectSite.height = matGoldenImg.rows;
	}

	int margin_h, margin_v;// templ horz and vert margin
	
	Rect  templRect = rectSite;	//用于对正的模板区域
	margin_h = sAlignmentRange.iMarginH;		// 与BaseGolden的最大允许偏差
	margin_v = sAlignmentRange.iMarginV;     // 原始数值128    
	templRect.x = margin_h;
	templRect.y = margin_v;
	templRect.width = matGoldenImg.cols - margin_h*2;
	templRect.height = matGoldenImg.rows - margin_v*2;
	//margin_h = templRect.x;
	//margin_v = templRect.y;

	int match_method;
	//match_method = CV_TM_SQDIFF;
	match_method = CV_TM_SQDIFF_NORMED;

	Mat templ, result;
	templ = Mat::zeros(templRect.height, templRect.width, matGoldenImg.type());
	matBaseGoldenImg(templRect).copyTo(templ);

	//cuda::GpuMat gTempl, gResult, gCurrImg;
	//gTempl.upload(templ);
	//gResult.upload(result);

	int result_cols = matGoldenImg.cols - templ.cols + 1;
	int result_rows = matGoldenImg.rows - templ.rows + 1;
	result.create( result_rows, result_cols, CV_32FC1 );

	vector<Mat>::iterator itImg = vecmatDstImg.begin();
	Mat sum, cur, curMatch;
	sum = Mat::zeros(matGoldenImg.rows, matGoldenImg.cols, CV_32FC1);
	matBaseGoldenImg.convertTo(sum, CV_32FC1);
	int dx, dy;

	for ( ; itImg != vecmatDstImg.end(); ++itImg)
	{
		matchTemplate( *itImg, templ, result, match_method );
		//normalize( result, result, 0, 1, NORM_MINMAX, -1, Mat() );

		double minVal; double maxVal; Point minLoc; Point maxLoc;
		Point matchLoc;
		minMaxLoc( result, &minVal, &maxVal, &minLoc, &maxLoc, Mat() );

		dx = minLoc.x - margin_h;
		dy = minLoc.y - margin_v;
		printf("golden match shift: x=%d, y=%d, %4.2f\n", dx, dy, minVal);

		//gCurrImg.upload(*itImg);
		
		Rect  srcRect, dstRect;	//对正区域

		if(dx >= 0)
		{
			srcRect.x = dx;			
			dstRect.x = 0;			
		}
		else
		{
			srcRect.x = 0;
			dstRect.x = -dx;		
		}
		srcRect.width = matGoldenImg.cols - abs(dx);
		dstRect.width = srcRect.width;

		if(dy >= 0)
		{
			srcRect.y = dy;
			dstRect.y = 0;
		}
		else
		{
			srcRect.y = 0;
			dstRect.y = -dy;
		}
		srcRect.height = matGoldenImg.rows - abs(dy);
		dstRect.height = srcRect.height;

		(*itImg).convertTo(cur, CV_32FC1);
		curMatch = Mat::zeros(matGoldenImg.rows, matGoldenImg.cols, CV_32FC1);
		cur(srcRect).copyTo(curMatch(dstRect));

		sum += curMatch;
	}

	sum = sum / (vecmatDstImg.size()+1);
	sum.convertTo(matGoldenImg, CV_8U);
}
bool DefectDetection::MakeGoldenSite(
	vector<cv::Mat>& vecmatDstImg,
	cv::Rect rectSite,
	cv::Mat matBaseGoldenImg,
	cv::Mat& matGoldenImg)
{
	int iShiftX;
	int	iShiftY;
	unsigned int nShiftCount = 0;

	if (vecmatDstImg.empty() || matGoldenImg.empty())
	{
	    //AfxMessageBox("vecmatDstImg.empty() || matGoldenImg.empty()!");
		return false;
	}

	if (rectSite.width <= 0 || rectSite.height <= 0)
	{
		rectSite.x = 0;
		rectSite.y = 0;
		rectSite.width = matGoldenImg.cols;
		rectSite.height = matGoldenImg.rows;
	}

    Mat matTemp(matBaseGoldenImg.size(), CV_8UC1);
    Mat matSum(matBaseGoldenImg.size(), CV_32FC1, Scalar(0));
    Mat matBase(matBaseGoldenImg.size(), CV_32FC1, Scalar(0));
	//imshow("matBaseGoldenImg", matBaseGoldenImg);
	//waitKey(1);
	matBaseGoldenImg.convertTo(matSum, CV_32FC1);
    //matBaseGoldenImg.convertTo(matBase, CV_32FC1);
	matSum /= 256.0;
    //matDebug = 0;


	//vecmatDiffRegions.clear();
	vector<cv::Mat>::iterator itmatDstImgVec = vecmatDstImg.begin();

    for (;itmatDstImgVec != vecmatDstImg.end(); ++itmatDstImgVec)
	{
		Mat matTmpRegion(itmatDstImgVec->size(), CV_8U);

        if (!GetDiffRegions(*itmatDstImgVec, rectSite, matGoldenImg, matTmpRegion, iShiftX, iShiftY, matBaseGoldenImg))
		{
			cout << "GetDiffRegions() - batch fail!" << endl;
            //AfxMessageBox("GetDiffRegions() - batch fail!");
			return false;
		}		
        
		//vecmatDiffRegions.push_back(matTmpRegion);
		if (0 == iShiftX && 0 == iShiftY)
        {
            //AfxMessageBox("0 == iShiftX && 0 == iShiftY");
            (*itmatDstImgVec).convertTo(matBase, CV_32FC1);
			matSum += (matBase / 256.0);
			printf("golden match shift: x=%d, y=%d \n", iShiftX, iShiftY);
        }
		else
		{
			nShiftCount++;
		    //char szTemp[255];
            //sprintf(szTemp, "!(0 == iShiftX && 0 == iShiftY) %d %d", iShiftX, iShiftY);
		    //AfxMessageBox(szTemp);
            //putText(matDebug, szTemp, Point(10,10), FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255));
            //imshow("matDebug", matDebug);

			printf("golden match shift: x=%d, y=%d \n", iShiftX, iShiftY);
            
            Translate(*itmatDstImgVec, matTemp, iShiftX, iShiftY);
            matTemp.convertTo(matBase, CV_32FC1);
            matSum += (matBase / 256.0);
		}
	}

    if (0 != vecmatDstImg.size())
    {
        //matSum = matSum / vecmatDstImg.size() * 256;
        matSum = matSum / (vecmatDstImg.size() + 1) * 256;
        //imwrite("matSum.jpg", matSum);
		//matTemp = matSum;
        //imshow("matSum", matSum);
		//waitKey(1);
    }

    matSum.convertTo(matGoldenImg, CV_8UC1);
    
	return true;
}

// ------------------------------------------------------------
// Description : 批量的图像配准和相减
// Parameters :  vecmatDstImg - 目标图片
//				 matGoldenImg - 金标准图片
//				 vecmatDiffRegions - 配准相减后的图片
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectDetection::GetBatchDiffRegions(
	vector<cv::Mat>& vecmatDstImg,
	cv::Rect rectSite,
	const cv::Mat& matGoldenImg,
	vector<cv::Mat>& vecmatDiffRegions)
{
	int iShiftX;
	int	iShiftY;

	if (vecmatDstImg.empty() || matGoldenImg.empty())
	{
		return false;
	}

	if (rectSite.width <= 0 || rectSite.height <= 0)
	{
		rectSite.x = 0;
		rectSite.y = 0;
		rectSite.width = matGoldenImg.cols;
		rectSite.height = matGoldenImg.rows;
	}

	vecmatDiffRegions.clear();
	vector<cv::Mat>::iterator itmatDstImgVec = vecmatDstImg.begin();

    for (;itmatDstImgVec != vecmatDstImg.end(); ++itmatDstImgVec)
	{
		Mat matTmpRegion(itmatDstImgVec->size(), CV_8U);

        if (!GetDiffRegions(*itmatDstImgVec, rectSite, matGoldenImg, matTmpRegion, iShiftX, iShiftY))
		{
			cout << "GetDiffRegions() - batch fail!" << endl;
			return false;
		}		
        
		vecmatDiffRegions.push_back(matTmpRegion);
	}
    
	return true;
}


// ------------------------------------------------------------
// Description : 图像配准和相减
// Parameters :  matDstImg - 目标图片
//				 matGoldenImg - 金标准图片
//				 matDiffRegions - 配准相减后的图片
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectDetection::GetDiffRegions(
	cv::Mat& matDstImg,
	cv::Rect rectSite,
	const cv::Mat& matGoldenImg, 
	cv::Mat& matDiffRegions,
	int& iShiftX,
	int& iShiftY)
{
	if (matDstImg.empty() || matGoldenImg.empty())
	{
		return false;
	}

	int iShift = 5;	// 2 
	//int iSumTmp = std::numeric_limits<int>::max();
    int iSumTmp = 1088 * 2048 * 255;
	
	for (int s1 = -iShift; s1 <= iShift; s1++)
    {   
		for (int s2 = -iShift; s2 <= iShift; s2++) 
		{
			Mat matOut = Mat::zeros(matDstImg.size(), matDstImg.type());

            matDstImg(
                Rect(iShift - s1, iShift - s2, matDstImg.cols - 2 * iShift, matDstImg.rows - 2 * iShift))
                .copyTo(matOut(cv::Rect(iShift, iShift, matDstImg.cols - 2 * iShift, matDstImg.rows - 2 * iShift)));

            Mat matDiff = abs(matGoldenImg - matOut);
			int iSumC = sum(matDiff(Rect(iShift, iShift, matDstImg.cols - 2 * iShift, matDstImg.rows - 2 * iShift)))[0];

            if (iSumC < iSumTmp) 
			{
				matDiffRegions = Mat::zeros(matDstImg.size(), matDstImg.type()); 
				matDiff(rectSite).copyTo(matDiffRegions(rectSite));
				iSumTmp = iSumC;	
                iShiftX = s1;
                iShiftY = s2;
			}
		}
    }
    
	return true;
}

bool DefectDetection::GetDiffRegions(
	cv::Mat& matDstImg,
	cv::Rect rectSite,
	const cv::Mat& matGoldenImg, 
	cv::Mat& matDiffRegions,
	int& iShiftX,
	int& iShiftY,
	cv::Mat matBaseGoldenImg)
{
	if (matDstImg.empty() || matGoldenImg.empty())
	{
		return false;
	}

	int iShift = 50;	// 2 
	//int iSumTmp = std::numeric_limits<int>::max();
    int iSumTmp = 1088 * 2048 * 255;
    //matBaseGoldenImg.copyTo(matGoldenImg);

    Mat matOut = Mat::zeros(matDstImg.size(), matDstImg.type());
	
	for (int s1 = -iShift; s1 <= iShift; s1++)
    {   
		for (int s2 = -iShift; s2 <= iShift; s2++) 
		{
			matOut = 0;

            matDstImg(
                Rect(iShift - s1, iShift - s2, matDstImg.cols - 2 * iShift, matDstImg.rows - 2 * iShift))
                .copyTo(matOut(cv::Rect(iShift, iShift, matDstImg.cols - 2 * iShift, matDstImg.rows - 2 * iShift)));

            Mat matDiff = abs(matBaseGoldenImg - matOut);
			int iSumC = sum(matDiff(Rect(iShift, iShift, matDstImg.cols - 2 * iShift, matDstImg.rows - 2 * iShift)))[0];

            //Mat matDiff0 = abs(matBaseGoldenImg - matDstImg);
            //imshow("matDiff0", matDiff0);
            //imshow("matOut", matOut);
            //imshow("matDiff", matDiff);
			//imshow("matBaseGoldenImg", matBaseGoldenImg);
            //imshow("matGoldenImg", matGoldenImg);
			//imshow("matDstImg", matDstImg);
			//waitKey(1);

            if (iSumC < iSumTmp) 
			{
				matDiffRegions = Mat::zeros(matDstImg.size(), matDstImg.type()); 
				matDiff(rectSite).copyTo(matDiffRegions(rectSite));
				iSumTmp = iSumC;	
                iShiftX = s1;
                iShiftY = s2;
                
                //imshow("matDiffRegions", matDiffRegions);
                //char szTemp[255];
                //sprintf(szTemp, "iSumC = %d", iSumC);
                //putText(matDebug, szTemp, Point(10, 50), FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255));
                //imshow("matDebug", matDebug);
			}
		}
    }
    
	return true;
}


bool DefectDetection::GetDiffRegions(
	cv::Mat& dstImg,
	cv::Rect siteRect,
	const cv::Mat& goldenImg,
	const cv::Mat& maskImg,
	cv::Mat& DiffRegionsFg,
	cv::Mat& DiffRegionsBg,
	cv::Mat& DiffRegions,
	int& iShiftX,
	int& iShiftY,
	MatchType matchtype)
{
// 	LARGE_INTEGER litmp;
// 	LONGLONG p1, p2;
// 	double dFreq, dTime, t[20];
// 	QueryPerformanceFrequency(&litmp);
// 	dFreq = (double)litmp.QuadPart;

	if (dstImg.empty() || goldenImg.empty() || maskImg.empty())
	{
		return false;
	}
	
	Mat diffMatFg = Mat::zeros(dstImg.size(), dstImg.type());
	Mat diffMatBg = Mat::zeros(dstImg.size(), dstImg.type());
	Mat dstRef = Mat::zeros(dstImg.size(), dstImg.type());
	Mat dstRefMin = Mat::zeros(dstImg.size(), dstImg.type());
	Mat diff, maskInvImg;
	int regionWidth, regionHeight;
	Rect regionRect, cuRefRect;	//计算diff的区域

	maskInvImg = ~maskImg;

	int off_x, off_y;
	Rect  templRect = siteRect;	//对正模板区域
	off_x = 64;
	off_y = 64;
	templRect.x = off_x;
	templRect.y = off_y;
	templRect.width = goldenImg.cols - off_x*2;
	templRect.height = goldenImg.rows - off_y*2;

	int match_method;
	//match_method = CV_TM_SQDIFF;CV_TM_SQDIFF_NORMED
	match_method = CV_TM_SQDIFF_NORMED;

	int mid_dx = 0, mid_dy = 0;
	Match(goldenImg, dstImg, mid_dx, mid_dy, matchtype);

// 	QueryPerformanceCounter(&litmp);
// 	p2 = litmp.QuadPart;
// 	t[0] = (double)(p2 - p1) / dFreq;
// 	p1 = p2;
// 	cout << "match cost time : " << t[0] << endl;

	cuRefRect = siteRect;
	cuRefRect.x += mid_dx;
	cuRefRect.y += mid_dy;
	if(cuRefRect.x < 0)
	{
		cuRefRect.width += ((cuRefRect.width + cuRefRect.x) - goldenImg.cols);
		siteRect.width = cuRefRect.width;
		cuRefRect.x = 0;	//ToDo: 偏差过大，应该报错
	}
	if(cuRefRect.y < 0)
	{
		cuRefRect.height += ((cuRefRect.y + cuRefRect.height) - goldenImg.rows);
		siteRect.height = cuRefRect.height;
		cuRefRect.y = 0;	//ToDo: 偏差过大，应该报错
	}

	if ((cuRefRect.x + cuRefRect.width) > goldenImg.cols)
	{
		cuRefRect.width -= ((cuRefRect.width + cuRefRect.x) - goldenImg.cols);
		siteRect.width = cuRefRect.width;
	}
	if ((cuRefRect.y + cuRefRect.height) > goldenImg.rows)
	{
		cuRefRect.height -= ((cuRefRect.y + cuRefRect.height) - goldenImg.rows);
		siteRect.height = cuRefRect.height;
	}

	regionRect = siteRect;
	diffMatFg = Mat::zeros(dstImg.size(), dstImg.type());
	diffMatBg = Mat::zeros(dstImg.size(), dstImg.type());
	dstRef = Mat::zeros(dstImg.size(), dstImg.type());
	dstImg(cuRefRect).copyTo(dstRef(regionRect));

	dstRefMin = min(dstRef, goldenImg);	//前景，只计算goldendie上是light，dstImg是dark的点
	diff = abs(goldenImg - dstRefMin);
	diff = diff & maskImg;
	diff(regionRect).copyTo(diffMatFg(cuRefRect));	//foreground 差异图

	dstRefMin = max(dstRef, goldenImg);	//背景，只计算goldendie上是dark，dstImg是light的点
	diff = abs(goldenImg - dstRefMin);
	diff = diff & maskInvImg;
	diff(regionRect).copyTo(diffMatBg(cuRefRect));	//background 区域 图

	Mat goldenRef = dstImg.clone();
	goldenImg(regionRect).copyTo(goldenRef(cuRefRect));
	DiffRegions = abs(goldenRef - dstImg);

	iShiftX = mid_dx;// minLoc.x - off_x;
	iShiftY = mid_dy;// minLoc.y - off_y;

	DiffRegionsFg = Mat::zeros(dstImg.size(), dstImg.type());
	diffMatFg(siteRect).copyTo(DiffRegionsFg(siteRect));

	DiffRegionsBg = Mat::zeros(dstImg.size(), dstImg.type());
	diffMatBg(siteRect).copyTo(DiffRegionsBg(siteRect));
// 	QueryPerformanceCounter(&litmp);
// 	p2 = litmp.QuadPart;
// 	t[0] = (double)(p2 - p1) / dFreq;
// 	p1 = p2;
// 	cout << "match next cost time : " << t[0] << endl;

	return true;
}

void DefectDetection::Translate(cv::Mat const& matSrc, cv::Mat& matDst, int iShiftX, int iShiftY)
{
    CV_Assert(matSrc.depth() == CV_8U);

    const int iRows = matSrc.rows;
    const int iCols = matSrc.cols;

    matDst.create(iRows, iCols, matSrc.type());

    uchar *pbyRow;
    
	for (int i = 0; i < iRows; i++)
    {
        pbyRow = matDst.ptr(i);

        for (int j = 0; j < iCols; j++)
        {
            // 平移后坐标映射到原图像
            int x = j - iShiftX;
            int y = i - iShiftY;

            // 保证映射后的坐标在原图像范围内
			if (x >= 0 && y >= 0 && x < iCols && y < iRows)
			{
				pbyRow[j] = matSrc.ptr(y)[x];
			}
        }
    }
}


BOOL DefectDetection::Match(const cv::Mat Temp, const cv::Mat Samp, int &dx, int &dy, MatchType matchtype)
{
	if (Temp.empty() || Samp.empty())
		return FALSE;

	int off_x = 64;
	int off_y = 64;
	int match_method = CV_TM_SQDIFF_NORMED;
	dx = 0, dy = 0;

	if (FASTEST == matchtype) {
		//水平投影和竖直投影的匹配效果
		vector<double> HorProTemp, VerProTemp, HorProSamp, VerProSamp;
		int HorMeanTemp, HorMeanSamp, i = 0;
		GrayProjections(Temp, cvRect(off_x, off_y, Temp.cols - 2 * off_x, Temp.rows - 2 * off_y), HorProTemp, VerProTemp, HorMeanTemp);
		GrayProjections(Samp, cvRect(0, 0, Samp.cols, Samp.rows), HorProSamp, VerProSamp, HorMeanSamp);

		VectorMatch( VerProSamp, VerProTemp, off_x, dx);
		VectorMatch( HorProSamp, HorProTemp, off_y, dy);

// 		double dMin = std::numeric_limits<double>::min();
// 		int width = Samp.cols;
// 		int hight = Samp.rows;
// 		long x = off_x, y = off_y;
// 		double dAbsSum = 0.0f;
// 		std::vector<double> matchSamp;
// 		for (i = 0; i < 2 * off_y; i++) {
// 			dAbsSum = 0.0f;
// 			matchSamp.assign(HorProSamp.begin() + i, HorProSamp.begin() + (i + hight - 2 * off_y));
// 
// 			dAbsSum = VtRelativity(matchSamp, HorProTemp);
// 			if (dAbsSum > dMin) {
// 				dMin = dAbsSum;
// 				y = i;
// 			}
// 		}
// 
// 		dMin = std::numeric_limits<double>::min();
// 		for (i = 0; i < 2 * off_x; i++) {
// 			dAbsSum = 0.0f;
// 			matchSamp.assign(VerProSamp.begin() + i, VerProSamp.begin() + (i + width - 2 * off_x));
// 
// 			dAbsSum = VtRelativity(matchSamp, VerProTemp);
// 			if (dAbsSum > dMin) {
// 				dMin = dAbsSum;
// 				x = i;
// 			}
// 		}
// 
// 		dx = x - off_x;
// 		dy = y - off_y;

//		SpliteVectorMatch(VerProTemp, VerProSamp, off_x, dx);
//		SpliteVectorMatch(HorProTemp, HorProSamp, off_y, dy);
//		outfile << "fastest: dx = " << dx << " dy = " << dy << "\n";
	}

	if (GENERAL == matchtype) {
		//四角对准
		cv::Size dstCutSz, goldenCutSz;
		dstCutSz.width = Temp.cols / 4;
		dstCutSz.height = Temp.rows / 4;
		goldenCutSz.width = Temp.cols / 4 - 2 * off_x;
		goldenCutSz.height = Temp.rows / 4 - 2 * off_y;

		if (dstCutSz.width > off_x * 2 && dstCutSz.height > off_y * 2) {
			//取四个角的图像作为mask
			int x[4] = { 0 }, y[4] = { 0 }, local_dx[4] = { 0 }, local_dy[4] = { 0 };
			int minAbs = 1000, minAbsDx;
			std::vector<int> valid_dx, valid_dy;
			std::vector<cv::Rect> dstCutRect, goldenCutRect;
			std::vector<cv::Mat> dstImgCut, goldenImgCut;
			dstCutRect.push_back(cvRect(0, 0, dstCutSz.width, dstCutSz.height));
			dstCutRect.push_back(cvRect(Temp.cols * 3.0 / 4, 0, dstCutSz.width, dstCutSz.height));
			dstCutRect.push_back(cvRect(Temp.cols * 3.0 / 4, Temp.rows * 3.0 / 4, dstCutSz.width, dstCutSz.height));
			dstCutRect.push_back(cvRect(0, Temp.rows * 3.0 / 4, dstCutSz.width, dstCutSz.height));

			goldenCutRect.push_back(cvRect(off_x, off_y, goldenCutSz.width, goldenCutSz.height));
			goldenCutRect.push_back(cvRect(Temp.cols * 3.0 / 4 + off_x, off_y, goldenCutSz.width, goldenCutSz.height));
			goldenCutRect.push_back(cvRect(Temp.cols * 3.0 / 4 + off_x, Temp.rows * 3.0 / 4 + off_y, goldenCutSz.width, goldenCutSz.height));
			goldenCutRect.push_back(cvRect(off_x, Temp.rows * 3.0 / 4 + off_y, goldenCutSz.width, goldenCutSz.height));

			cv::Mat result;
			double minVal; double maxVal; Point minLoc; Point maxLoc;
			int result_cols = dstCutSz.width - goldenCutSz.width + 1;
			int result_rows = dstCutSz.height - goldenCutSz.height + 1;
			result.create(result_rows, result_cols, CV_32FC1);

			for (int i = 0; i < dstCutRect.size(); i++) {
				cv::Mat tempCut = cv::Mat::zeros(dstCutSz, Samp.type());
				Samp(dstCutRect[i]).copyTo(tempCut);
				dstImgCut.push_back(tempCut);

				cv::Mat SampCut = cv::Mat::zeros(goldenCutSz, Samp.type());
				Temp(goldenCutRect[i]).copyTo(SampCut);
				goldenImgCut.push_back(SampCut);

				matchTemplate(dstImgCut[i], goldenImgCut[i], result, match_method);
				minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());
				local_dx[i] = minLoc.x - off_x;
				local_dy[i] = minLoc.y - off_y;

				if (minAbs > cv::abs(local_dx[i])) {
					minAbs = cv::abs(local_dx[i]);
					minAbsDx = local_dx[i];
				}
				//				std::cout << "dx[ " << i << " ] = " << dx[i] << ", dy[ " << i << " ] = " << dy[i] << endl;
			}

			//选取dx，dy中的有效值存放在valid_dx和valid_dy中
			for (int i = 0; i < 4; i++) {
				if (local_dx[i] > minAbsDx - 5 && local_dx[i] < minAbsDx + 5) {
					valid_dx.push_back(local_dx[i]);
					valid_dy.push_back(local_dy[i]);
				}
//				outfile << "valid_dx = " << local_dx[i] << ", valid_dy = " << local_dy[i] << "\n";
			}

			//排序valid_dx和valid_dy
			for (int i = 0; i < valid_dx.size(); i++) {
				for (int j = i; j < valid_dx.size(); j++) {
					if (valid_dx[i] > valid_dx[j]) {
						valid_dx[i] += valid_dx[j];
						valid_dx[j] = valid_dx[i] - valid_dx[j];
						valid_dx[i] -= valid_dx[j];
					}

					if (valid_dy[i] > valid_dy[j]) {
						valid_dy[i] += valid_dy[j];
						valid_dy[j] = valid_dy[i] - valid_dy[j];
						valid_dy[i] -= valid_dy[j];
					}
				}
			}

			//选取valid_dx和valid_dy的中值做为全图的平移向量
			if (valid_dx.size() != 0) {
				if (valid_dx.size() % 2 == 1) {
					dx = valid_dx[valid_dx.size() / 2];
					dy = valid_dy[valid_dy.size() / 2];
				}
				else {
					dx = (valid_dx[valid_dx.size() / 2 - 1] + valid_dx[valid_dx.size() / 2]) / 2;
					dy = (valid_dy[valid_dy.size() / 2 - 1] + valid_dy[valid_dy.size() / 2]) / 2;
				}
			}
		}

//		outfile << "general: dx = " << dx << " dy = " << dy << "\n";
	}


	if (MOSTACCURATE == matchtype) {
		//全图对准
		Rect  templRect;	
		off_x = 64;
		off_y = 64;
		templRect.x = off_x;
		templRect.y = off_y;
		templRect.width = Temp.cols - off_x * 2;
		templRect.height = Temp.rows - off_y * 2;

		Mat templ, result;
		templ = Mat::zeros(Temp.rows, Temp.cols, Temp.type());
		Temp(templRect).copyTo(templ);

		int result_cols = Temp.cols - templ.cols + 1;
		int result_rows = Temp.rows - templ.rows + 1;
		result.create(result_rows, result_cols, CV_32FC1);

		matchTemplate(Samp, templ, result, match_method);
		//normalize( result, result, 0, 1, NORM_MINMAX, -1, Mat() );

		double minVal; double maxVal; Point minLoc; Point maxLoc;
		Point matchLoc;
		minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, Mat());

		dx = minLoc.x - off_x;
		dy = minLoc.y - off_y;
	}
	return TRUE;
}


BOOL DefectDetection::GrayProjections(const cv::Mat src, cv::Rect rc, std::vector<double> &HorPro, std::vector<double> &VerPro, int &HorProMean)
{
	if (src.empty() || rc.width > src.cols || rc.height > src.rows || rc.width < 0 || rc.height < 0) return FALSE;

	int width = src.cols;
	int hight = src.rows;
	HorPro.clear();
	VerPro.clear();

	cv::Mat rcSrc = cv::Mat::zeros(rc.height, rc.width, src.type());
	src(rc).copyTo(rcSrc);

	double ArrayRow[3000] = { 0 }, ArrayCol[3000] = { 0 };
	for (int i = 0; i < rc.height; i++) {
		for (int j = 0; j < rc.width; j++) {
			ArrayRow[i] += (double)rcSrc.at<uchar>(i, j);
			ArrayCol[j] += (double)rcSrc.at<uchar>(i, j);
		}
	}

	HorProMean = 0;
	for (int i = 0; i < rc.height; i++) {
		ArrayRow[i] = ArrayRow[i] * 1.0 / rc.width;
		HorPro.push_back(ArrayRow[i]);

		HorProMean += ArrayRow[i];
	}
	HorProMean /= rc.height;

	for (int i = 0; i < rc.width; i++) {
		ArrayCol[i] = ArrayCol[i] * 1.0 / rc.height;
		VerPro.push_back(ArrayCol[i]);
	}

	return TRUE;
}


BOOL DefectDetection::VectorMatch(std::vector<double> samp, std::vector<double> temp, int off, int &ndelta)
{
	if (samp.size() < temp.size() + 2 * off - 1) {
		return FALSE;
	}

	double dDev = 0.0f;
	double dMinDev = std::numeric_limits<double>::min();
	int nSize = temp.size();
	vector<double> matchSamp;

	for (int j = 0; j < 2 * off; j++) {
		matchSamp.assign(samp.begin() + j, samp.begin() + nSize + j);
		dDev = VtRelativity(matchSamp, temp);
		if (dDev > dMinDev) {
			dMinDev = dDev;
			ndelta = j - off;
		}
	}
	return TRUE;
}


BOOL DefectDetection::SpliteVectorMatch(std::vector<double> temp, std::vector<double> samp, int off, int &delta)
{
	if (temp.size() + 2 * off != samp.size()) {
		return FALSE;
	}

	int lenTemp = temp.size();
	int nSeq = 4;                        //均等分为几段匹配
	int nStep = lenTemp / nSeq;
	vector<int> vtSeq;

	for (int i = 1; i <= nSeq - 1; i++) {
		vtSeq.push_back((i - 1) * nStep);
		vtSeq.push_back(i * nStep - 1);
	}
	vtSeq.push_back((nSeq - 1) * nStep);
	vtSeq.push_back(lenTemp - 1);

	double dMinDev = -1.0f;// std::numeric_limits<double>::min();
	int temp_x;
	double Dev;
	vector<double> valid_temp, matchSamp, Dif;
	vector<int> allpos;


// 	ofstream ofs;
// 	ofs.open("3.txt");
// 	ofs << "8888**********************************" << "\n";
	for (int i = 1; i <= nSeq; i++) { 
		valid_temp.assign(temp.begin() + vtSeq[2 * (i - 1)], temp.begin() + vtSeq[2 * (i - 1) + 1]);
		dMinDev = -1.0f;// std::numeric_limits<double>::min();
		for (int j = 0; j < 2 * off - 1; j++) {
			matchSamp.assign(samp.begin() + vtSeq[2 * (i - 1)] + j, samp.begin() + vtSeq[2 * (i - 1) + 1] + j);
			Dev = VtRelativity(matchSamp, valid_temp);
			if (Dev > dMinDev) {
				dMinDev = Dev;
				temp_x = j - off;
			}
		}
		allpos.push_back(temp_x);

//		ofs << "\n" << allpos[0] << " " << allpos[1] << " " << allpos[2] << " " << allpos[3] << " " << dMinDev << "\n";
 	}

	int tempdis;
	for (int i = 0; i <= nSeq - 2; i++) {
		for (int j = i; j <= nSeq - 1; j++) {
			if (allpos[i] > allpos[j]) {
				tempdis = allpos[i];
				allpos[i] = allpos[j];
				allpos[j] = tempdis;
			}
		}
	}
	//去中间两个绝对值较小的作为基础偏移
	int nDisBeg = abs(allpos[0] - allpos[1]);
	int nDisMedian = abs(allpos[1] - allpos[2]);
	int nDisEnd = abs(allpos[2] - allpos[3]);
	if (nDisMedian < 8 && nDisBeg > 8 && nDisEnd > 8) {
		delta = (allpos[1] + allpos[2]) / 2;
	}
	else {
		int nBasePos = 0;
		vector<int> abspos;
		for (int i = 0; i < allpos.size(); i++) {
			abspos.push_back(abs(allpos[i]));
		}

		//选取最小绝对值的真实值为nBasePos
		int tempdis;
		for (int i = 0; i <= nSeq - 2; i++) {
			for (int j = i; j <= nSeq - 1; j++) {
				if (abspos[i] > abspos[j]) {
					tempdis = abspos[i];
					abspos[i] = abspos[j];
					abspos[j] = tempdis;

					tempdis = allpos[i];
					allpos[i] = allpos[j];
					allpos[j] = tempdis;
				}
			}
		}
		nBasePos = allpos[0];

		tempdis = 0;
		int nCount = 0;
		for (int i = 0; i < nSeq; i++) {
			if (abs(allpos[i] - nBasePos) < 8) {
				tempdis += allpos[i];
				nCount++;
			}
		}
		delta = tempdis * 1.0 / nCount + 0.5;
	}

//	ofs << allpos[0] << " " << allpos[1] << " " << allpos[2] << " " << allpos[3] << "\n";
//	ofs.close();
}


vector<double> DefectDetection::VtSub(vector<double> samp, vector<double> temp)
{
	vector<double> result;
	int nSize = min(samp.size(), temp.size());
	for (int i = 0; i < nSize; i++) {
		result.push_back(samp[i] - temp[i]);
	}
	return result;
}

double DefectDetection::VtDev(vector<double> vt)
{
	int nSize = vt.size();
	double sum = 0.0;
	for (int i = 0; i < nSize; i++) {
		sum += vt[i];
	}
	double dMean = sum / nSize;

	sum = 0.0f;
	for (int i = 0; i < nSize; i++) {
		sum += ((vt[i] - dMean) * (vt[i] - dMean));
	}
	return sum / nSize;
}

double DefectDetection::VtMean(std::vector<double> vt)
{
	if (vt.empty()) {
		return 0.0f;
	}

	double dMean = 0.0f;
	for (int i = 0; i < vt.size(); i++) {
		dMean += vt[i];
	}
	return dMean / vt.size();
}


std::vector<double> DefectDetection::VtSmooth(std::vector<double> vt, int kerWid)
{
	int nRaduis = kerWid / 2 + 1;
	vector<double> res;

	//读取前半个kerWid的原始数据
	for (int i = 0; i < nRaduis; i++) {
		res.push_back(vt[i]);
	}

	vector<double> samp;
	double dNum = 0.0f;
	//中间段滤波
	for (int i = nRaduis; i < vt.size() - nRaduis; i++) {
		dNum = 0.0f;
		for (int j = i - nRaduis; j <= i + nRaduis; j++) {
			dNum += vt[j];
		}
		dNum /= (2 * nRaduis + 1);
		res.push_back(dNum);
	}

	//读取后半个kerWid的原始数据
	for (int i = vt.size() - nRaduis; i < vt.size(); i++) {
		res.push_back(vt[i]);
	}

	return res;
}


double DefectDetection::VtRelativity(std::vector<double> samp, std::vector<double> temp)
{
	if (samp.size() != temp.size()) {
		return 0.0f;
	}
	int nSize = min(temp.size(), samp.size());

	vector<double> VtProd;
	for (int i = 0; i < nSize; i++) {
		VtProd.push_back(samp[i] * temp[i]);
	}

	double EXY = VtMean(VtProd);
	double EX = VtMean(samp);
	double EY = VtMean(temp);
	double VarX = VtDev(samp);
	double VarY = VtDev(temp);
	double Den = sqrt(VarX * VarY);
	return Den > -0.00001 && Den < 0.00001 ? 0.0f : (EXY - EX * EY) / Den;
}