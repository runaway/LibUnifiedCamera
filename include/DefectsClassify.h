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
	// Description : ȱ�ݷ���
	// Parameters :  anomaliesMat: �������
	//				 strategyList: Strategy 
	//				 outputResult: �����ֵ��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
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
	// Description : ����ȱ�������������λ�����أ�
	// Parameters :   anomaliesMat: �������
	//				 srcImg: ԭʼͼ��
	//				 outputData: �������ȱ���������
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool CalcDefectsParam(
		cv::Mat& anomaliesMat,
		cv::Mat& srcImg,
		float fPixelSizeRatio,
		std::list< DefectParam<float> >& outputData);

	// ------------------------------------------------------------
	// Description : ����Strategy�Ĳ�������
	// Parameters :  inputData: ��ȱ������Ĳ���
	//				 strategyList: strategy
	//				 outputResult: ����������
	// Return Value :true - �ɹ�
	//				 false - ʧ��
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


