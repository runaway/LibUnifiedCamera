#pragma once

#include "ComDef.h"
#include <list>
#include <vector>
#include <opencv2/opencv.hpp>
#include "DefectBinningStrategy.h"
#include <Map>

typedef std::map<std::string, cv::Scalar> ColorMap;

class DefectsClassify
{
public:
	DefectsClassify();
	~DefectsClassify();

	// ------------------------------------------------------------
	// Description : 缺陷分类
	// Parameters :  anomaliesMat: 检出区域
	//				 strategyList: Strategy 
	//				 outputResult: 【输出值】
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool DoClassify(
		cv::Mat& anomaliesMat,
		cv::Mat& srcImg,		
		float fPixelSizeRatio,
		std::string strategyName,
		cv::Mat& resultMat,
		std::vector<defectDescription<float> >& outputResult);

	bool DoClassify(	
		std::vector< std::vector<cv::Point> >& defectsContours,
		cv::Mat& srcImg,	
		cv::Mat& buffer,
		float fPixelSizeRatio,
		std::string strategyName,
		cv::Mat& resultMat,
		std::vector<defectDescription<float> >& outputResult);

private:

	// ------------------------------------------------------------
	// Description : 计算缺陷区域参数（单位：像素）
	// Parameters :   anomaliesMat: 检出区域
	//				 srcImg: 原始图像
	//				 outputData: 【输出】缺陷区域参数
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool CalcDefectsParam(
		cv::Mat& anomaliesMat,
		cv::Mat& srcImg,
		float fPixelSizeRatio,
		std::list< DefectParam<float> >& outputData);

	// ------------------------------------------------------------
	// Description : 根据Strategy的参数分类
	// Parameters :  inputData: 各缺陷区域的参数
	//				 strategyList: strategy
	//				 outputResult: 【输出】结果
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool Classify(
		std::string strategyName,
		std::list< DefectParam<float> >& inputData,
		StrategyList& strategyList, 
		std::vector< defectDescription<float> >& outputResult,
		cv::Mat& outputMat);

	bool CalcDefectsParam(// 
		std::vector< std::vector<cv::Point> >& defectsContours,
		cv::Mat& srcImg,
		cv::Mat& buffer,
		float fPixelSizeRatio,
		std::list< DefectParam<float> >& outputData);

	bool Classify(// 
		std::vector< std::vector<cv::Point> >& defectsContours,
		std::string strategyName,
		std::list< DefectParam<float> >& inputData,
		StrategyList& strategyList, 
		std::vector< defectDescription<float> >& outputResult,
		cv::Mat& outputMat);

private:
	std::vector< std::vector<cv::Point> > m_defectsContours;
	ColorMap m_colorMap;

public:
	cv::Mat m_matClustering;
};


