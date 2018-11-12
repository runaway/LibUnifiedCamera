#include "stdafx.h"
#include "ROIUI.h"
#include <iostream>
#include <highgui.h>
#include <functional>


using namespace cv;
using namespace std;


static bool gbIsDrawing = false;					// �Ƿ����ڻ���ROI
static Scalar siteColor = Scalar(0 ,255, 255);		// site������ɫ
static Scalar ROIColor = Scalar(0, 0, 255);			// ROI��������ɫ


// ------------------------------------------------------------
// Description : ROI����¼��ص�����
// Parameters : 
// Return Value :
// ------------------------------------------------------------
void ROIMouseCallBack(int nEvent, int x, int y, int flags, void*param)
{
	// ֻ��Ӧ���������º��ƶ�����Ϣ����������µ�����ƶ���Ϣ����Ӧ
	if (CV_EVENT_MOUSEMOVE == nEvent)
	{
		if (!gbIsDrawing || !(flags & CV_EVENT_LBUTTONDOWN))
		{
			return;
		}		
	}
	else
	{
		gbIsDrawing = (CV_EVENT_LBUTTONDOWN == nEvent);
	}
		
	ROIUI* pROIUI = (ROIUI*)param;
	pROIUI->ResponseROIDrawing(nEvent, x, y);
}


ROIUI::ROIUI() :
	m_pROIParamRecords(NULL),
	m_srcImg(NULL),
	m_ROIShape(ROI_RECTANGLE),
	m_anchorPoint(0, 0),
	m_nROIRadius(0),
	m_ROIRect(0, 0, 0, 0),
	m_fWidRatio(0),
	m_fHeiRatio(0),
	m_SiteRect(0, 0, 0, 0),
	m_bDrawSiteRegion(true),
	m_curRecordIdx(1),
	m_configName("")
{	
	// ������ӳ���ʼ��
	m_methodMap.insert(make_pair(ROI_CIRCLE, &ROIUI::ResponseDrawCircle));
	m_methodMap.insert(make_pair(ROI_RECTANGLE, &ROIUI::ResponseDrawRectangle));
	m_methodMap.insert(make_pair(ROI_FREEHAND_LINE, &ROIUI::ResponseDrawFreehandLine));
}

ROIUI::~ROIUI()
{	
	if (NULL != m_pROIParamRecords)
	{
		delete m_pROIParamRecords;
	}
	m_ROITracePointVec.clear();
}

// ------------------------------------------------------------
// Description : ����site�ļ������
// Parameters :  srcImg�����Ƶ�ͼ��
//				 nWid: ԭͼ���
//				 nHei: ԭͼ�߶�
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool ROIUI::DrawSiteRegion(cv::Mat& srcImg, int nWid, int nHei)
{
	if (srcImg.empty() || nWid <= 0 || nHei <= 0)
	{
		return false;
	}

	if (NULL != m_pROIParamRecords 
		&& (m_srcImg->cols != srcImg.cols
			|| m_srcImg->rows != srcImg.rows))
	{
		delete m_pROIParamRecords;
		m_pROIParamRecords = NULL;
		
	}
	if (m_pROIParamRecords == NULL)
	{
		m_pROIParamRecords = new SiteROIParamRecords(srcImg.cols, srcImg.rows);
	}

	m_fWidRatio = (float)nWid / srcImg.cols;
	m_fHeiRatio = (float)nHei / srcImg.rows;
	m_bDrawSiteRegion = true;
	m_ROIShape = ROI_RECTANGLE;
	m_srcImg = &srcImg;

	setMouseCallback(SHOW_WIN_NAME, ROIMouseCallBack, (void*)this);
}

// ------------------------------------------------------------
// Description : ��ȡsite�������
// Parameters :  ��
// Return Value :site�������
// ------------------------------------------------------------
Rect ROIUI::GetSiteRect()
{	
	Rect realSizeRect;

	realSizeRect.x = m_SiteRect.x * m_fWidRatio;
	realSizeRect.y = m_SiteRect.y * m_fHeiRatio;
	realSizeRect.width = m_SiteRect.width * m_fWidRatio;
	realSizeRect.height = m_SiteRect.height * m_fHeiRatio;

	cout << "m_fWidRatio = " << m_fWidRatio << endl;
	cout << "m_fHeiRatio = " << m_fHeiRatio << endl;
	return realSizeRect;
}

// ------------------------------------------------------------
// Description : �û�����ROI
// Parameters :  srcImg: ���Ƶ�ͼ��
//				 roiShape: roi��״
//				 configName: ROI��Ӧ�������ļ�������
//				 nWid: ԭͼ���
//				 nHei: ԭͼ�߶�
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool ROIUI::DrawROI(cv::Mat& srcImg, ROIShape roiShape,	
	std::string& configName, int nWid, int nHei)
{	
	if (srcImg.empty() || nWid <= 0 || nHei <= 0)
	{
		return false;
	}

 	m_fWidRatio = (float)nWid / srcImg.cols;
	m_fHeiRatio = (float)nHei / srcImg.rows;
	m_bDrawSiteRegion = false;

	if (NULL != m_pROIParamRecords
		&& (m_srcImg->cols != srcImg.cols
			|| m_srcImg->rows != srcImg.rows))
	{
		delete m_pROIParamRecords;
		m_pROIParamRecords = NULL;

	}
	if (m_pROIParamRecords == NULL)
	{
		m_pROIParamRecords = new SiteROIParamRecords(srcImg.cols, srcImg.rows);
	}

	m_ROIShape = roiShape;
	m_srcImg = &srcImg;
	m_configName = configName;

	setMouseCallback(SHOW_WIN_NAME, ROIMouseCallBack, (void*)this);
// 	while (1)
// 	{
// 		if (waitKey(20) == 27)
// 		{
// 			break;
// 		}
// 	}
	return true;
}

// ------------------------------------------------------------
// Description : �Ծ���ROI��Χ��������
// Parameters :  ROIRect������ROI��Χ
//				 nSrcWid: ͼ����
//				 nSrcHei: ͼ��߶�
// Return Value :��
// ------------------------------------------------------------
void ROIUI::LimitROIRect(Rect& ROIRect, int nSrcWid, int nSrcHei)
{
	if (ROIRect.x + ROIRect.width > nSrcWid)
	{
		ROIRect.width = nSrcWid - ROIRect.x;
	}
	if (ROIRect.y + ROIRect.height > nSrcHei)
	{
		ROIRect.height = nSrcHei - ROIRect.y;
	}
}

// ------------------------------------------------------------
// Description : ��ǰ��¼������һ�������¼
// Parameters :  curRegionRecord: ��ǰ�û����Ƶ�����
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool ROIUI::AddNewParamToRecord(Mat& curRegionRecord)
{
	if (NULL == m_pROIParamRecords)
	{
		return false;
	}

	// ȡ�������¼����
	Mat recordRegions;
	if (!m_pROIParamRecords->GetROIRegionMat(recordRegions))
	{
		return false;
	}
	
	Mat intersectRegion;

	// ��ǰ���������֮ǰ��¼������Ľ���
	bitwise_and(recordRegions, curRegionRecord, intersectRegion);
	//imwrite("intersectRegion.bmp", intersectRegion);

	vector < vector < cv::Point > > contourTmp;
	vector<Vec4i> hierarchy;
	// �ҳ���������
	findContours(intersectRegion, contourTmp, hierarchy, CV_RETR_LIST,
		CV_CHAIN_APPROX_SIMPLE);

	// ������������Ϊ0˵����֮ǰ���Ƶ�������ڽ�������ǰ�û����Ƶ�������Ч
	if (contourTmp.size() > 0)
	{
		return false;
	}

	// ������������Ϊ0˵����֮ǰ���Ƶ����򲻴��ڽ���������ǰ�û������������
	// ����ļ�¼
	bitwise_or(recordRegions, curRegionRecord, recordRegions);
	
	ROIParam roiParam;
	roiParam.configName = m_configName;
	roiParam.shape = m_ROIShape;

	if (m_ROIShape == ROI_RECTANGLE)
	{
		roiParam.data.rect.x = m_ROIRect.x * m_fWidRatio;
		roiParam.data.rect.y = m_ROIRect.y * m_fHeiRatio;
		roiParam.data.rect.width = m_ROIRect.width * m_fWidRatio;
		roiParam.data.rect.height = m_ROIRect.height * m_fHeiRatio;
	}
	else if (m_ROIShape == ROI_CIRCLE)
	{
		roiParam.data.circle.x = m_anchorPoint.x * m_fWidRatio;
		roiParam.data.circle.y = m_anchorPoint.y * m_fHeiRatio;
		roiParam.data.circle.radius = m_nROIRadius * m_fWidRatio;
	}
	else if (m_ROIShape == ROI_FREEHAND_LINE)
	{
		vector<Point>::iterator it = roiParam.data.contour.begin();
		for (; it != roiParam.data.contour.end(); ++it)
		{
			Point tmpPoint;
			tmpPoint.x = it->x * m_fWidRatio;
			tmpPoint.y = it->y * m_fHeiRatio;
			roiParam.data.contour.push_back(tmpPoint);
		}
	}
	m_pROIParamRecords->InsertNewRecord(m_curRecordIdx, roiParam);

	++m_curRecordIdx;
	return true;
}

// ------------------------------------------------------------
// Description : ����¼���Ӧ
// Parameters :  nEvent: �¼�
//				 x�����x����ƫ����
//				 y�����y����ƫ����
// Return Value :true: �����ҵ�ӳ�亯�����ɹ�
//				 false: �����ҵ�ӳ�亯����ʧ��
// ------------------------------------------------------------
bool ROIUI::ResponseROIDrawing(int nEvent, int x, int y)
{
	map<ROIShape, FuncPtr>::iterator it = m_methodMap.find(m_ROIShape);
	if (it == m_methodMap.end())
	{
		return false;
	}
	FuncPtr pFunc = (it->second);
	(this->*pFunc)(nEvent, x, y);
	return true;
}

// ------------------------------------------------------------
// Description : �ػ�site�������
// Parameters :  dstMat: ����Ŀ��
// Return Value :true - site���������Ч(�ѳ�ʼ��)
//				 false - site���������Ч
// ------------------------------------------------------------
bool ROIUI::RedrawSiteRegion(Mat& dstMat)
{
	if (m_SiteRect.width == 0 || m_SiteRect.height == 0)
	{
		return false;
	}
	cv::rectangle(dstMat, m_SiteRect, siteColor, 2);
	return true;
}

// ------------------------------------------------------------
// Description : �ػ����ROI����
// Parameters :  dstMat: ����Ŀ��
// Return Value :��
// ------------------------------------------------------------
void ROIUI::RedrawROI(Mat& dstMat)
{	
	Mat recordRegions;
	if (NULL == m_pROIParamRecords  
		|| !m_pROIParamRecords->GetROIRegionMat(recordRegions))
	{
		return;
	}
	
	vector < vector < cv::Point > > contourTmp;
	vector<Vec4i> hierarchy;
	findContours(recordRegions, contourTmp, hierarchy, CV_RETR_LIST,
		CV_CHAIN_APPROX_SIMPLE);

	// ֮ǰû��ROI�����¼�������ػ�
	if (contourTmp.size() == 0)
	{
		return;
	}

	drawContours(dstMat, contourTmp, -1, ROIColor);	
}

// ------------------------------------------------------------
// Description : ����Բ����Ϣ��Ӧ
// Parameters : 
// Return Value :
// ------------------------------------------------------------
void ROIUI::ResponseDrawCircle(int nEvent, int x, int y)
{
	Mat dispMat = Mat::zeros(m_srcImg->rows, m_srcImg->cols, CV_8UC3);
	m_srcImg->copyTo(dispMat);

	if (CV_EVENT_LBUTTONDOWN == nEvent)
	{
		m_anchorPoint.x = x;
		m_anchorPoint.y = y;
		m_nROIRadius = 1;
		cv::circle(dispMat, m_anchorPoint, m_nROIRadius, ROIColor);
	}
	else if (CV_EVENT_MOUSEMOVE == nEvent)
	{
		m_nROIRadius = sqrt(pow(m_anchorPoint.x - x, 2) +
			pow(m_anchorPoint.y - y, 2));

		// ����Բ��
		cv::circle(dispMat, m_anchorPoint, 1, ROIColor);
		// ����Բ
		cv::circle(dispMat, m_anchorPoint, m_nROIRadius, ROIColor);
	}
	else if (CV_EVENT_LBUTTONUP == nEvent)
	{
		// ��¼��ǰ���Ƶ�����
		Mat curRegionRecord = Mat::zeros(m_srcImg->rows, m_srcImg->cols,
			CV_8UC1);
		// ����ǰ�������򱣴���һ����ʱMat��
		cv::circle(curRegionRecord, m_anchorPoint, m_nROIRadius,
			Scalar(m_curRecordIdx), -1);
		// �жϵ�ǰ�����֮ǰ���Ƶ�ROI���������ཻ��������ཻ������ǰ���򵱳���Ч
		// ���򣬲���ӵ�ROI record ��
		AddNewParamToRecord(curRegionRecord);		
	}

	RedrawSiteRegion(dispMat);
	RedrawROI(dispMat);
	imshow(SHOW_WIN_NAME, dispMat);
}

// ------------------------------------------------------------
// Description : ���ƾ��ε���Ϣ��Ӧ
// Parameters : 
// Return Value :
// ------------------------------------------------------------
void ROIUI::ResponseDrawRectangle(int nEvent, int x, int y)
{
	Mat dispMat = Mat::zeros(m_srcImg->rows, m_srcImg->cols, CV_8UC3);
	m_srcImg->copyTo(dispMat);

	Rect* rect = m_bDrawSiteRegion ? &m_SiteRect : &m_ROIRect;
	Scalar color = m_bDrawSiteRegion ? siteColor : ROIColor;
	
	if (CV_EVENT_LBUTTONDOWN == nEvent)
	{
		rect->x = x;
		rect->y = y;
		rect->width = 1;
		rect->height = 1;
		cv::rectangle(dispMat, *rect, color);
	}
	else if (CV_EVENT_MOUSEMOVE == nEvent)
	{
		int nWid = x - m_ROIRect.x;
		int nHei = y - m_ROIRect.y;

		if (nWid < 0 || nHei < 0)
		{
			return;
		}
		rect->width = nWid;
		rect->height = nHei;
		LimitROIRect(*rect, m_srcImg->cols, m_srcImg->rows);

		cv::rectangle(dispMat, *rect, color);
	}
	else if (CV_EVENT_LBUTTONUP == nEvent)
	{
		// �жϵ�ǰ�����֮ǰ���Ƶ�ROI���������ཻ��������ཻ������ǰ���򵱳���Ч
		// ���򣬲���ӵ�ROI record ��
		if (!m_bDrawSiteRegion)
		{
			// ��¼��ǰ���Ƶ�����
			Mat curRegionRecord = Mat::zeros(m_srcImg->rows, m_srcImg->cols, CV_8UC1);
			// ����ǰ�������򱣴���һ����ʱMat��
			cv::rectangle(curRegionRecord, *rect, Scalar(m_curRecordIdx), -1);			
			AddNewParamToRecord(curRegionRecord);
		}	
		else
		{
			m_pROIParamRecords->SetSiteRegion(m_SiteRect);
			cv::rectangle(dispMat, *rect, color, 2);
		}
	}

	RedrawSiteRegion(dispMat);
	RedrawROI(dispMat);
	imshow(SHOW_WIN_NAME, dispMat);
}

// ------------------------------------------------------------
// Description : �����������ߵ���Ϣ��Ӧ
// Parameters : 
// Return Value :
// ------------------------------------------------------------
void ROIUI::ResponseDrawFreehandLine(int nEvent, int x, int y)
{
	Mat dispMat = Mat::zeros(m_srcImg->rows, m_srcImg->cols, CV_8UC3);
	m_srcImg->copyTo(dispMat);

	if (CV_EVENT_LBUTTONDOWN == nEvent)
	{
		m_ROITracePointVec.clear();
		m_ROITracePointVec.push_back(Point(x, y));
		cv::circle(dispMat, Point(x, y), 1, ROIColor);
	}
	else if (CV_EVENT_MOUSEMOVE == nEvent)
	{
		m_ROITracePointVec.push_back(Point(x, y));
		vector<cv::Point>::iterator it = m_ROITracePointVec.begin() + 1;
		while (it != m_ROITracePointVec.end())
		{
			Point startPoint = *(it - 1);
			Point endPoint = *it;
			line(dispMat, startPoint, endPoint, ROIColor);
			++it;
		}
	}
	else if (CV_EVENT_LBUTTONUP == nEvent)
	{
		// ��¼��ǰ���Ƶ�����
		Mat curRegionRecord = Mat::zeros(m_srcImg->rows, m_srcImg->cols, CV_8UC1);

		vector<vector<Point>> contour;
		contour.push_back(m_ROITracePointVec);

		// ����ǰ�������򱣴���һ����ʱMat��
		drawContours(curRegionRecord, contour, 0, Scalar(m_curRecordIdx), -1);

		// �жϵ�ǰ�����֮ǰ���Ƶ�ROI���������ཻ��������ཻ������ǰ���򵱳���Ч
		// ���򣬲���ӵ�ROI record ��
		AddNewParamToRecord(curRegionRecord);
	}

	RedrawSiteRegion(dispMat);
	RedrawROI(dispMat);
	imshow(SHOW_WIN_NAME, dispMat);
}

// ------------------------------------------------------------
// Description : ����������ļ�
// Parameters :  path: ��������·��
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool ROIUI::SaveRecordToFile(char* path)
{
	if (NULL == m_pROIParamRecords)
	{
		return false;
	}
		
	m_pROIParamRecords->SetSiteRegion(GetSiteRect());
	return m_pROIParamRecords->SaveRecordsToFile(path);
}