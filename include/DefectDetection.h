#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <windows.h>
#include "ComDef.h"
#include "DefectsAnalysis.h"

class DefectDetection
{
public:	
	
	// ------------------------------------------------------------
	// Description : 缺陷检测
	// Parameters :  dstImg: 待检测图片
	//				 goldenImgPath: 创建金标准图片的路径
	//				 defectRegions: 【输出】待检测图片与金标准图片相减后的
	//				 图片
	//				 bWithRotate: 是否需要旋转匹配
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool Detection(
		cv::Mat& dstImg, 
		const cv::Mat& goldenImg, 
		const cv::Mat& maskImg,
		cv::Rect siteRect,
		unsigned char thresholdVal,
		cv::Mat& defectRegionsFg,
		cv::Mat& defectRegionsBg,
		cv::Mat& diffMat,
		bool bWithRotate,
		MatchType matchtype = FASTEST);

	// ------------------------------------------------------------
	// Description : 批量的图像配准和相减
	// Parameters :  dstAImgVec - 目标图片
	//				 goldenImg - 金标准图片
	//				 DiffRegionsVec - 配准相减后的图片
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool GetBatchDiffRegions(
		std::vector<cv::Mat>& dstImgVec,
		cv::Rect siteRect,
		const cv::Mat& goldenImg,
		std::vector<cv::Mat>& DiffRegionsVec);

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
		std::vector<std::string>& imgNameVec,
		const std::string& dstImgPath,
		const cv::Mat& goldenImg, 
		unsigned char thresholdVal,
		cv::Rect siteRect,
		std::vector<cv::Mat>& defectRegionsVec,
		bool bParallel,
		bool bWithRotate);

	// ------------------------------------------------------------
	// Description : 创建golden image
	// Parameters :  goldenImgPath: 创建金标准图片的路径
	//				 goldenImg: 金标准图片
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool CreateGoldenImage(
		const std::string& goldenImgPath,
		cv::Mat& goldenImg);

	bool CreateGoldenImage(
		const std::string& strGoldenImgPath,
		cv::Mat& matGoldenImg,
		cv::Mat matBaseImg,
		AlignmentRange sAlignmentRange);

	bool CreateGoldenMaskImage(
		const cv::Mat& goldenImg,
		cv::Mat& goldenMaskImg);

	bool DefectDetection::CreateGoldenSite(
		std::vector<cv::Mat>& vecmatDstImg,
		cv::Rect rectSite,
		cv::Mat matBaseGoldenImg,
		cv::Mat& matGoldenImg,
		AlignmentRange sAlignmentRange);

private:
	// ------------------------------------------------------------
	// Description : 图像配准和相减
	// Parameters :  dstImg - 目标图片
	//				 goldenImg - 金标准图片
	//				 DiffRegions - 配准相减后的图片
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool GetDiffRegions(
		cv::Mat& dstImg,
		cv::Rect siteRect,
		const cv::Mat& goldenImg,
		cv::Mat& DiffRegions,
		int& iShiftX,
		int& iShiftY);

	bool GetDiffRegions(
		cv::Mat& dstImg,
		cv::Rect siteRect,
		const cv::Mat& goldenImg,
		cv::Mat& DiffRegions,
		int& iShiftX,
		int& iShiftY,
		cv::Mat matBaseGoldenImg);

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
		MatchType matchtype = FASTEST);

	bool MakeGoldenSite(
		std::vector<cv::Mat>& vecmatDstImg,
		cv::Rect rectSite,
		cv::Mat matBaseGoldenImg,
		cv::Mat& matGoldenImg);

	void Translate(cv::Mat const& matSrc, cv::Mat& matDst, int iShiftX, int iShiftY);

	BOOL Match(const cv::Mat Temp, const cv::Mat Samp, int &dx, int &dy, MatchType matchtype = FASTEST);
	BOOL GrayProjections(const cv::Mat src, cv::Rect rc, std::vector<double> &HorPro, std::vector<double> &VerPro, int &HorProMean);
	BOOL VectorMatch(std::vector<double> vt1, std::vector<double> vt2, int off, int &ndelta);
	BOOL SpliteVectorMatch(std::vector<double> vt1, std::vector<double> vt2, int off, int &ndelta);

	std::vector<double> VtSub(std::vector<double> samp, std::vector<double> temp);
	double VtDev(std::vector<double> vt);
	double VtMean(std::vector<double> vt);
	std::vector<double> VtSmooth(std::vector<double> vt, int kerWid);

	double VtRelativity(std::vector<double> samp, std::vector<double> temp);
};

