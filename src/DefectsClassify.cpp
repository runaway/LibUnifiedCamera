#include "stdafx.h"
#include "DefectsClassify.h"


using namespace cv;
using namespace std;


DefectsClassify::DefectsClassify()
{
	// 初始化color map——临时方案，后面写成配置文件
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
// Description : 缺陷分类
// Parameters :  anomaliesMat: 检出区域
//				 strategyList: Strategy 
//				 outputResult: 【输出值】
// Return Value :true - 成功
//				 false - 失败
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
	
	//-----------------------------------  计算缺陷区域参数
	std::list<DefectParam<float> > defectParamList;

    if (!CalcDefectsParam(anomaliesMat, srcImg, fPixelSizeRatio, defectParamList))
	{
	    //AfxMessageBox("CalcDefectsParam Fail!");
		return false;
	}
		
	//-----------------------------------  根据Strategy将缺陷分类并绘制缺陷轮廓线	
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
// Description : 根据Strategy的参数分类
// Parameters :  inputData: 各缺陷区域的参数
//				 strategyList: strategy
//				 outputResult: 【输出】结果
// Return Value :true - 成功
//				 false - 失败
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
		
		// 找出当前缺陷参数满足的bin
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
        
		// 如果没找到，也保存在结果列表里，但是binName设置为空			
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

		// 绘制缺陷的轮廓线
		Scalar color = m_colorMap[defectDes.className];		
		drawContours(outputMat, m_defectsContours, id, color, 2);

		// 绘制ID
		string strID = to_string(id + 1);
		cv::Point2f center;
		float radius;
		minEnclosingCircle(m_defectsContours[id], center, radius);
		putText(outputMat, strID, center, FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 255));
	}

	return true;
}




// ------------------------------------------------------------
// Description : 计算缺陷区域参数（单位：像素）
// Parameters :   anomaliesMat: 检出区域
//				 srcImg: 原始图像
//				 outputData: 【输出】缺陷区域参数
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool DefectsClassify::CalcDefectsParam(
	Mat& anomaliesMat, 
	Mat& srcImg, 	
	float fPixelSizeRatio,
	std::list< DefectParam<float> >& outputData)
{
	m_defectsContours.clear();

	// 找出所有区域的轮廓	
	findContours(anomaliesMat, m_defectsContours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	//namedWindow("anomaliesMat", 1);
	//imshow("anomaliesMat", anomaliesMat);
	//waitKey(1);

	if (m_defectsContours.empty())
	{
		return false;
	}

	outputData.clear();

	// 遍历求出各个轮廓的面积、长宽比、对应的灰度值、尺寸，
	// 存放在容器outputData中
	int nContourNum = m_defectsContours.size();

    for (int i = 0; i < nContourNum; ++i)
	{
		DefectParam<float> data;

		// 计算面积
		data.area = contourArea(m_defectsContours[i]) * fPixelSizeRatio;
		
		// 用最小外接矩形计算长度/宽度/长宽比		
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

		// 计算中心点		
		data.centerX = boundingBox.center.x * fPixelSizeRatio;
		data.centerY = boundingBox.center.y * fPixelSizeRatio;

		// 求缺陷区域平均灰度
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
// Description : 缺陷分类
// Parameters : 
//				 strategyList: Strategy 
//				 outputResult: 【输出值】
// Return Value :true - 成功
//				 false - 失败
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
	//-----------------------------------  计算缺陷区域参数
	std::list<DefectParam<float> > defectParamList;

    if (!CalcDefectsParam(defectsContours, srcImg, buffer, fPixelSizeRatio, defectParamList))
	{
	    //AfxMessageBox("CalcDefectsParam Fail!");
		return false;
	}
		
	//-----------------------------------  根据Strategy将缺陷分类并绘制缺陷轮廓线	
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
// Description : 根据Strategy的参数分类
// Parameters :  inputData: 各缺陷区域的参数
//				 strategyList: strategy
//				 outputResult: 【输出】结果
// Return Value :true - 成功
//				 false - 失败
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
		
		// 找出当前缺陷参数满足的bin
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
        
		// 如果没找到，也保存在结果列表里，但是binName设置为空			
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
			// 绘制缺陷的轮廓线
			Scalar color = m_colorMap[defectDes.className];		
			drawContours(outputMat, defectsContours, id, color, 1);
            drawContours(m_matClustering, defectsContours, id, color, FILLED); 
			#if 0
			// 绘制ID
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
// Description : 计算缺陷区域参数
// Parameters : 
//				 srcImg: 原始图像
//				 outputData: 【输出】缺陷区域参数
// Return Value :true - 成功
//				 false - 失败
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

	// 遍历求出各个轮廓的面积、长宽比、对应的灰度值、尺寸，
	// 存放在容器outputData中
	int nContourNum = defectsContours.size();

    for (int i = 0; i < nContourNum; ++i)
	{
		DefectParam<float> data;

		// 计算面积
		data.area = contourArea(defectsContours[i]) * fPixelSizeRatio;
		
		// 用最小外接矩形计算长度/宽度/长宽比		
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

		// 计算中心点		
		data.centerX = boundingBox.center.x * fPixelSizeRatio;
		data.centerY = boundingBox.center.y * fPixelSizeRatio;
		data.angle = boundingBox.angle;

		Rect boundRect = boundingRect(defectsContours[i]);
		Mat singleRegion = buffer(boundRect);
		singleRegion = Mat::zeros(singleRegion.size(), singleRegion.type());

		// 求缺陷区域平均灰度
		drawContours(buffer, defectsContours, i, Scalar(255), CV_FILLED);
		Scalar meanGray = mean(srcImg(boundRect), singleRegion);
	
		data.brightness = meanGray[0];

		outputData.push_back(data);
	}
    
	return true;
}

