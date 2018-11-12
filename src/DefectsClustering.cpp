#include "stdafx.h"
#include "DefectsClustering.h"
#include "DefectBinningStrategy.h"


using namespace cv;
using namespace std;

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
// Description : �����ռ���࣬���������ȱ�ݵ���һ��ȱ��
//				 fRadius: ����뾶
//				 pixelSizeRatio: ÿ���ض�Ӧ������ߴ�
//				 defectRegions: ȱ������
//				 outputResult: �����ֵ��������������clusterID��
// Return Value :true - �ɹ�
//				 false - ʧ��
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
	
	// ��ÿ��ȱ���������һ��ID
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

		// ��һ���Ե�ǰ��������
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
// Description : ����binning strategy�ľ���
// Parameters :  strategyName: Strategy����
//				 fRadius: ����뾶
//				 pixelSizeRatio: ÿ���ض�Ӧ������ߴ�
//				 defectRegions: ȱ������
//				 outputResult: �����ֵ��������������clusterID��
// Return Value :true - �ɹ�
//				 false - ʧ��
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
// Description : ����ROIģ��ľ���
// Parameters :  ROIMaskName��ROIģ�������
//				 fRadius: ����뾶
//				 pixelSizeRatio: ÿ���ض�Ӧ������ߴ�
//				 defectRegions: ȱ������
//				 outputResult: �����ֵ��������������clusterID��
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool DefectsClustering::DoROIClustering(const std::string& ROIMaskName,
	float fRadius, float pixelSizeRatio, cv::Mat& defectRegions,
	std::vector<defectDescription<float> >& outputResult)
{
	// TODO
	return true;
}
