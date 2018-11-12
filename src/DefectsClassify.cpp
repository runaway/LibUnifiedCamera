#include "stdafx.h"
#include "DefectsClassify.h"


using namespace cv;
using namespace std;


DefectsClassify::DefectsClassify()
{
	// ��ʼ��color map������ʱ����������д�������ļ�
	m_colorMap.insert(make_pair("", Scalar(255, 0, 255)));
	m_colorMap.insert(make_pair("class1", Scalar(0, 0, 255)));
	m_colorMap.insert(make_pair("class2", Scalar(0, 255, 0)));
	m_colorMap.insert(make_pair("class3", Scalar(0, 128, 255)));
	m_colorMap.insert(make_pair("class4", Scalar(255, 255, 0)));
	m_colorMap.insert(make_pair("class5", Scalar(255, 128, 0)));
	m_colorMap.insert(make_pair("class6", Scalar(128, 128, 255)));
	m_colorMap.insert(make_pair("class7", Scalar(128, 0, 255)));
}

DefectsClassify::~DefectsClassify()
{
	m_colorMap.clear();
}

// ------------------------------------------------------------
// Description : ȱ�ݷ���
// Parameters :  anomaliesMat: �������
//				 strategyList: Strategy 
//				 outputResult: �����ֵ��
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool DefectsClassify::DoClassify(
	cv::Mat& anomaliesMat,
	cv::Mat& srcImg,	
	float fPixelSizeRatio,
	std::string strategyName,	
	cv::Mat& resultMat,
	std::vector<defectDescription<float> >& outputResult)
{
	if (anomaliesMat.empty())
	{
	    //AfxMessageBox("anomaliesMat.empty Fail!");
		return false;
	}

	cvtColor(srcImg, resultMat, CV_GRAY2RGB);
	
	//-----------------------------------  ����ȱ���������
	std::list<DefectParam<float> > defectParamList;

    if (!CalcDefectsParam(anomaliesMat, srcImg, fPixelSizeRatio, defectParamList))
	{
	    //AfxMessageBox("CalcDefectsParam Fail!");
		return false;
	}
		
	//-----------------------------------  ����Strategy��ȱ�ݷ��ಢ����ȱ��������	
	DefectBinningStrategy defectStrategy;
	StrategyList strategyList;	
    
	if (!defectStrategy.GetStrategyList(strategyName, strategyList))
	{
	    //AfxMessageBox("defectStrategy.GetStrategyList Fail!");
		return false;
	}
    
	return Classify(strategyName, defectParamList, strategyList, outputResult, resultMat);
}


//----------------------------------- private

// ------------------------------------------------------------
// Description : ����Strategy�Ĳ�������
// Parameters :  inputData: ��ȱ������Ĳ���
//				 strategyList: strategy
//				 outputResult: ����������
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool DefectsClassify::Classify(
	std::string strategyName, 
	std::list< DefectParam<float> >& inputData,
	StrategyList& strategyList, 
	std::vector<defectDescription<float> >& outputResult,
	Mat& outputMat)
{
    bool bMeetBinCriteria;
    
	if (inputData.empty() || strategyList.empty())
	{
		return false;
	}
	outputResult.clear();
		
	std::list< DefectParam<float> >::iterator inputIt = inputData.begin();

    for (int id = 0; inputIt != inputData.end(); ++inputIt, ++id)
	{
		StrategyList::iterator strategyIt = strategyList.begin();
		
		// �ҳ���ǰȱ�ݲ��������bin
		bool foundBin = false;
		defectDescription<float> defectDes;

		//if (inputIt->area < 120) continue;
		//bMeetBinCriteria = true;

        for (; strategyIt != strategyList.end(); ++strategyIt)
		{
			if (bMeetBinCriteria = strategyIt->MeetBinCriteria(*inputIt))
			{	
				defectDes.binName = strategyIt->binName;
				foundBin = true;	
				break;
			}			
		}

        //if (!bMeetBinCriteria)
        {
            //continue;
        }
        
		// ���û�ҵ���Ҳ�����ڽ���б������binName����Ϊ��			
		if (!foundBin)
		{
			defectDes.binName = "";
			defectDes.className = "";
		}
		else
		{
			defectDes.className = strategyIt->className;
		}

		defectDes.strategyName = strategyName;	
		defectDes.defectData = *inputIt;
		defectDes.clusterID = 0;
		outputResult.push_back(defectDes);		

		// ����ȱ�ݵ�������
		Scalar color = m_colorMap[defectDes.className];		
		drawContours(outputMat, m_defectsContours, id, color, 2);

		// ����ID
		string strID = to_string(id + 1);
		cv::Point2f center;
		float radius;
		minEnclosingCircle(m_defectsContours[id], center, radius);
		putText(outputMat, strID, center, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 255));
	}

	return true;
}




// ------------------------------------------------------------
// Description : ����ȱ�������������λ�����أ�
// Parameters :   anomaliesMat: �������
//				 srcImg: ԭʼͼ��
//				 outputData: �������ȱ���������
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool DefectsClassify::CalcDefectsParam(
	Mat& anomaliesMat, 
	Mat& srcImg, 	
	float fPixelSizeRatio,
	std::list< DefectParam<float> >& outputData)
{
	m_defectsContours.clear();

	// �ҳ��������������	
	findContours(anomaliesMat, m_defectsContours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	//namedWindow("anomaliesMat", 1);
	//imshow("anomaliesMat", anomaliesMat);
	//waitKey(1);

	if (m_defectsContours.empty())
	{
		return false;
	}

	outputData.clear();

	// ��������������������������ȡ���Ӧ�ĻҶ�ֵ���ߴ磬
	// ���������outputData��
	int nContourNum = m_defectsContours.size();

    for (int i = 0; i < nContourNum; ++i)
	{
		DefectParam<float> data;

		// �������
		data.area = contourArea(m_defectsContours[i]) * fPixelSizeRatio;
		
		// ����С��Ӿ��μ��㳤��/���/�����		
		RotatedRect boundingBox = minAreaRect(m_defectsContours[i]);
		data.length = boundingBox.size.width * fPixelSizeRatio;
		data.width = boundingBox.size.height * fPixelSizeRatio;

		if (data.length < data.width)
		{
			float tmp = data.length;
			data.length = data.width;
			data.width = tmp;
		}

		data.aspectRatio = data.length / data.width;

		// �������ĵ�		
		data.centerX = boundingBox.center.x * fPixelSizeRatio;
		data.centerY = boundingBox.center.y * fPixelSizeRatio;

		// ��ȱ������ƽ���Ҷ�
		Mat singleRegion = Mat::zeros(srcImg.size(), CV_8U);
		drawContours(singleRegion, m_defectsContours, i, Scalar(255), CV_FILLED);
		Scalar meanGray = mean(srcImg, singleRegion);
		data.brightness = meanGray[0];

		outputData.push_back(data);
	}
    
	return true;
}
// following added at 2017.Dec.04

// ------------------------------------------------------------
// Description : ȱ�ݷ���
// Parameters : 
//				 strategyList: Strategy 
//				 outputResult: �����ֵ��
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool DefectsClassify::DoClassify(
	std::vector< std::vector<cv::Point> >& defectsContours,
	cv::Mat& srcImg,	
	cv::Mat& buffer,
	float fPixelSizeRatio,
	std::string strategyName,	
	cv::Mat& resultMat,
	std::vector<defectDescription<float> >& outputResult)
{
	//-----------------------------------  ����ȱ���������
	std::list<DefectParam<float> > defectParamList;

    if (!CalcDefectsParam(defectsContours, srcImg, buffer, fPixelSizeRatio, defectParamList))
	{
	    //AfxMessageBox("CalcDefectsParam Fail!");
		return false;
	}
		
	//-----------------------------------  ����Strategy��ȱ�ݷ��ಢ����ȱ��������	
	DefectBinningStrategy defectStrategy;
	StrategyList strategyList;	
    
	if (!defectStrategy.GetStrategyList(strategyName, strategyList))
	{
	    //AfxMessageBox("defectStrategy.GetStrategyList Fail!");
		return false;
	}
    
	return Classify(defectsContours, strategyName, defectParamList, strategyList, outputResult, resultMat);
}

// ------------------------------------------------------------
// Description : ����Strategy�Ĳ�������
// Parameters :  inputData: ��ȱ������Ĳ���
//				 strategyList: strategy
//				 outputResult: ����������
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool DefectsClassify::Classify(
	std::vector< std::vector<cv::Point> >& defectsContours,
	std::string strategyName, 
	std::list< DefectParam<float> >& inputData,
	StrategyList& strategyList, 
	std::vector<defectDescription<float> >& outputResult,
	Mat& outputMat)
{
    bool bMeetBinCriteria;
    
	if (inputData.empty() || strategyList.empty())
	{
		return false;
	}
	outputResult.clear();
	m_matClustering = Mat::zeros(outputMat.size(), outputMat.type());
		
	std::list< DefectParam<float> >::iterator inputIt = inputData.begin();

    for (int id = 0; inputIt != inputData.end(); ++inputIt, ++id)
	{
		StrategyList::iterator strategyIt = strategyList.begin();
		
		// �ҳ���ǰȱ�ݲ��������bin
		bool foundBin = false;
		defectDescription<float> defectDes;

		//if (inputIt->area < 120) continue;
		//bMeetBinCriteria = true;

        for (; strategyIt != strategyList.end(); ++strategyIt)
		{
			if (bMeetBinCriteria = strategyIt->MeetBinCriteria(*inputIt))
			{	
				defectDes.binName = strategyIt->binName;
				foundBin = true;	
				break;
			}			
		}

        //if (!bMeetBinCriteria)
        {
            //continue;
        }
        
		// ���û�ҵ���Ҳ�����ڽ���б������binName����Ϊ��			
		if (!foundBin)
		{
			defectDes.binName = "";
			defectDes.className = "";
		}
		else
		{
			defectDes.className = strategyIt->className;
		}

		defectDes.strategyName = strategyName;	
		defectDes.defectData = *inputIt;
		defectDes.clusterID = 0;
		outputResult.push_back(defectDes);

		if(false==outputMat.empty())
		{
			// ����ȱ�ݵ�������
			Scalar color = m_colorMap[defectDes.className];		
			drawContours(outputMat, defectsContours, id, color, 1);
            drawContours(m_matClustering, defectsContours, id, color, FILLED); 
			#if 0
			// ����ID
			string strID = to_string(id + 1);
			cv::Point2f center;
			float radius;
			minEnclosingCircle(defectsContours[id], center, radius);
			putText(outputMat, strID, center, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 0));
			#endif
		}
	}

	return true;
}

// ------------------------------------------------------------
// Description : ����ȱ���������
// Parameters : 
//				 srcImg: ԭʼͼ��
//				 outputData: �������ȱ���������
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool DefectsClassify::CalcDefectsParam(
	std::vector< std::vector<cv::Point> >& defectsContours,
	Mat& srcImg, 
	Mat& buffer,
	float fPixelSizeRatio,
	std::list< DefectParam<float> >& outputData)
{
	
	if (defectsContours.empty())
	{
		return false;
	}

	outputData.clear();

	// ��������������������������ȡ���Ӧ�ĻҶ�ֵ���ߴ磬
	// ���������outputData��
	int nContourNum = defectsContours.size();

    for (int i = 0; i < nContourNum; ++i)
	{
		DefectParam<float> data;

		// �������
		data.area = contourArea(defectsContours[i]) * fPixelSizeRatio;
		
		// ����С��Ӿ��μ��㳤��/���/�����		
		RotatedRect boundingBox = minAreaRect(defectsContours[i]);
		data.length = boundingBox.size.width * fPixelSizeRatio;
		data.width = boundingBox.size.height * fPixelSizeRatio;

		if (data.length < data.width)
		{
			float tmp = data.length;
			data.length = data.width;
			data.width = tmp;
		}

		data.aspectRatio = data.length / data.width;

		// �������ĵ�		
		data.centerX = boundingBox.center.x * fPixelSizeRatio;
		data.centerY = boundingBox.center.y * fPixelSizeRatio;
		data.angle = boundingBox.angle;

		Rect boundRect = boundingRect(defectsContours[i]);
		Mat singleRegion = buffer(boundRect);
		singleRegion = Mat::zeros(singleRegion.size(), singleRegion.type());

		// ��ȱ������ƽ���Ҷ�
		drawContours(buffer, defectsContours, i, Scalar(255), CV_FILLED);
		Scalar meanGray = mean(srcImg(boundRect), singleRegion);
	
		data.brightness = meanGray[0];

		outputData.push_back(data);
	}
    
	return true;
}

