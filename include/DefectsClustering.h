#pragma once

#include "ComDef.h"
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>


class DefectsClustering
{
public:

	// ------------------------------------------------------------
	// Description : ȱ�ݿռ����
	// Parameters :  type�����෽ʽ
	//					CLUSTER_NONE - ��������
	//					CLUSTER_ROI - ����ROI�ľ���
	//					CLUSTER_BINNING_STRATEGY - ����binning strategy
	//					�ľ���
	//				 ROIorStrategyName: ROIģ���Strategy������
	//				 fRadius: ����뾶
	//				 pixelSizeRatio: ÿ���ض�Ӧ������ߴ�
	//				 defectRegions: ȱ������
	//				 outputResult: �����ֵ��������������clusterID��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool DoClustering(clusterType type, const std::string& ROIorStrategyName,
		float fRadius, float pixelSizeRatio, cv::Mat& defectRegions,
		std::vector<defectDescription<float> >& outputResult);

private:

	// ------------------------------------------------------------
	// Description : �����ռ���࣬���������ȱ�ݵ���һ��ȱ��
	//				 fRadius: ����뾶
	//				 pixelSizeRatio: ÿ���ض�Ӧ������ߴ�
	//				 defectRegions: ȱ������
	//				 outputResult: �����ֵ��������������clusterID��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool DoSpacialClustering(float fRadius, float pixelSizeRatio,
		cv::Mat& defectRegions,	std::vector<defectDescription<float> >& outputResult);
	
	// ------------------------------------------------------------
	// Description : ����binning strategy�ľ���
	// Parameters :  strategyName: Strategy����
	//				 fRadius: ����뾶
	//				 pixelSizeRatio: ÿ���ض�Ӧ������ߴ�
	//				 defectRegions: ȱ������
	//				 outputResult: �����ֵ��������������clusterID��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool DoBinningClustering(const std::string& strategyName, 
		float fRadius, float pixelSizeRatio, cv::Mat& defectRegions,
		std::vector<defectDescription<float> >& outputResult);

	// ------------------------------------------------------------
	// Description : ����ROIģ��ľ���
	// Parameters :  ROIMaskName��ROIģ�������
	//				 fRadius: ����뾶
	//				 pixelSizeRatio: ÿ���ض�Ӧ������ߴ�
	//				 defectRegions: ȱ������
	//				 outputResult: �����ֵ��������������clusterID��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool DoROIClustering(const std::string& ROIMaskName,
		float fRadius, float pixelSizeRatio, cv::Mat& defectRegions,
		std::vector<defectDescription<float> >& outputResult);
};
