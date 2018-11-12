#include "stdafx.h"
#include "DefectsClustering.h"
#include "DefectBinningStrategy.h"


using namespace cv;
using namespace std;

// ------------------------------------------------------------
// Description : 缺陷空间聚类
// Parameters :  type：聚类方式
//					CLUSTER_NONE - 不做聚类
//					CLUSTER_ROI - 基于ROI的聚类
//					CLUSTER_BINNING_STRATEGY - 基于binning strategy
//					的聚类
//				 ROIorStrategyName: ROI模板或Strategy的名称
//				 fRadius: 聚类半径
//				 pixelSizeRatio: 每像素对应的物理尺寸
//				 defectRegions: 缺陷区域
//				 outputResult: 【输出值】结果（仅填充了clusterID）
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectsClustering::DoClustering(clusterType type, const std::string& ROIorStrategyName,
	float fRadius, float pixelSizeRatio, cv::Mat& defectRegions,
	std::vector<defectDescription<float> >& outputResult)
{
	if (type == CLUSTER_NONE)
	{
		return DoSpacialClustering(fRadius, pixelSizeRatio, defectRegions,
			outputResult);
	}
	else
	{
		if (type == CLUSTER_BINNING_STRATEGY)
		{
			return DoBinningClustering(ROIorStrategyName, fRadius, pixelSizeRatio,
				defectRegions, outputResult);
		}
		else
		{
			return DoROIClustering(ROIorStrategyName, fRadius, pixelSizeRatio,
				defectRegions, outputResult);
		}
	}
}

/////////////////////////////////////////////// private

// ------------------------------------------------------------
// Description : 单做空间聚类，不将聚类的缺陷当成一个缺陷
//				 fRadius: 聚类半径
//				 pixelSizeRatio: 每像素对应的物理尺寸
//				 defectRegions: 缺陷区域
//				 outputResult: 【输出值】结果（仅填充了clusterID）
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectsClustering::DoSpacialClustering(float fRadius, float pixelSizeRatio,
	cv::Mat& defectRegions, std::vector<defectDescription<float> >& outputResult)
{
	if (outputResult.empty())
	{
		return false;
	}

	vector< vector<Point> > contours;
	findContours(defectRegions, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
	
	if (contours.empty())
	{
		return false;
	}
	
	Mat labelMat = Mat::zeros(defectRegions.size(), CV_8U);
	
	// 将每个缺陷区域分配一个ID
	vector< vector<Point> >::iterator contoursIt = contours.begin();
	for (int id = 1; contoursIt != contours.end(); ++contoursIt, ++id)
	{
		drawContours(labelMat, contours, id - 1, id, -1);
	}
	
	std::vector<defectDescription<float> >::iterator outputResultIt
		= outputResult.begin();
	int nID = 1;
	int pixelRadius = fRadius * pixelSizeRatio;
	for (; outputResultIt != outputResult.end(); ++outputResultIt)
	{		
		if (outputResultIt->clusterID != 0)
		{
			continue;
		}

		// 造一个以当前区域中心
		Mat radiusMat = Mat::zeros(defectRegions.size(), CV_8UC1);
		int x = outputResultIt->defectData.centerX / pixelSizeRatio;
		int y = outputResultIt->defectData.centerY / pixelSizeRatio;
		circle(radiusMat, Point(x, y), pixelRadius, Scalar(255), -1);
				
		MatND hist;
		int channel[1] = {0};
		int histSize[1] = { 256 };
		float hranges[2] = {0, 255};	
		const float *ranges[1] = {hranges};
			
		calcHist(&labelMat, 1, channel, radiusMat, hist, 1, histSize, ranges);
				
		bool bIncreaseID = false;
		for (int i = 0; i < 256; ++i)
		{
			int nID = hist.at<float>(i) + 0.5;
			if (nID > 0)
			{
				outputResult[i].clusterID = nID;
				bIncreaseID = true;
			}
		}
		if (bIncreaseID)
		{
			++nID;
		}
	}
	return true;
}

// ------------------------------------------------------------
// Description : 基于binning strategy的聚类
// Parameters :  strategyName: Strategy名称
//				 fRadius: 聚类半径
//				 pixelSizeRatio: 每像素对应的物理尺寸
//				 defectRegions: 缺陷区域
//				 outputResult: 【输出值】结果（仅填充了clusterID）
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectsClustering::DoBinningClustering(const std::string& strategyName,
	float fRadius, float pixelSizeRatio, cv::Mat& defectRegions,
	std::vector<defectDescription<float> >& outputResult)
{
	DefectBinningStrategy defectStrategy;
	StrategyList strategyList;
	if (!defectStrategy.GetStrategyList(strategyName, strategyList))
	{
		return false;
	}
}

// ------------------------------------------------------------
// Description : 基于ROI模板的聚类
// Parameters :  ROIMaskName：ROI模板的名称
//				 fRadius: 聚类半径
//				 pixelSizeRatio: 每像素对应的物理尺寸
//				 defectRegions: 缺陷区域
//				 outputResult: 【输出值】结果（仅填充了clusterID）
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectsClustering::DoROIClustering(const std::string& ROIMaskName,
	float fRadius, float pixelSizeRatio, cv::Mat& defectRegions,
	std::vector<defectDescription<float> >& outputResult)
{
	// TODO
	return true;
}
