#pragma once

#include "ComDef.h"
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>


class DefectsClustering
{
public:

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
	bool DoClustering(clusterType type, const std::string& ROIorStrategyName,
		float fRadius, float pixelSizeRatio, cv::Mat& defectRegions,
		std::vector<defectDescription<float> >& outputResult);

private:

	// ------------------------------------------------------------
	// Description : 单做空间聚类，不将聚类的缺陷当成一个缺陷
	//				 fRadius: 聚类半径
	//				 pixelSizeRatio: 每像素对应的物理尺寸
	//				 defectRegions: 缺陷区域
	//				 outputResult: 【输出值】结果（仅填充了clusterID）
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool DoSpacialClustering(float fRadius, float pixelSizeRatio,
		cv::Mat& defectRegions,	std::vector<defectDescription<float> >& outputResult);
	
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
	bool DoBinningClustering(const std::string& strategyName, 
		float fRadius, float pixelSizeRatio, cv::Mat& defectRegions,
		std::vector<defectDescription<float> >& outputResult);

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
	bool DoROIClustering(const std::string& ROIMaskName,
		float fRadius, float pixelSizeRatio, cv::Mat& defectRegions,
		std::vector<defectDescription<float> >& outputResult);
};
