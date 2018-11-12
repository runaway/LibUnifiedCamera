#include "stdafx.h"
#include "ROIUI.h"
#include <iostream>
#include <highgui.h>
#include <functional>


using namespace cv;
using namespace std;


static bool gbIsDrawing = false;					// 是否正在绘制ROI
static Scalar siteColor = Scalar(0 ,255, 255);		// site外框的颜色
static Scalar ROIColor = Scalar(0, 0, 255);			// ROI轮廓的颜色


// ------------------------------------------------------------
// Description : ROI鼠标事件回调函数
// Parameters : 
// Return Value :
// ------------------------------------------------------------
void ROIMouseCallBack(int nEvent, int x, int y, int flags, void*param)
{
	// 只响应鼠标左键按下后移动的消息，其他情况下的鼠标移动消息不响应
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
	// 表驱动映射初始化
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
// Description : 绘制site的检测区域
// Parameters :  srcImg：绘制的图像
//				 nWid: 原图宽度
//				 nHei: 原图高度
// Return Value :true - 成功
//				 false - 失败
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
// Description : 获取site检测区域
// Parameters :  无
// Return Value :site检测区域
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
// Description : 用户绘制ROI
// Parameters :  srcImg: 绘制的图像
//				 roiShape: roi形状
//				 configName: ROI对应的配置文件的名称
//				 nWid: 原图宽度
//				 nHei: 原图高度
// Return Value :true - 成功
//				 false - 失败
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
// Description : 对矩形ROI范围进行限制
// Parameters :  ROIRect：矩形ROI范围
//				 nSrcWid: 图像宽度
//				 nSrcHei: 图像高度
// Return Value :无
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
// Description : 向当前记录表增加一条区域记录
// Parameters :  curRegionRecord: 当前用户绘制的区域
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool ROIUI::AddNewParamToRecord(Mat& curRegionRecord)
{
	if (NULL == m_pROIParamRecords)
	{
		return false;
	}

	// 取出区域记录矩阵
	Mat recordRegions;
	if (!m_pROIParamRecords->GetROIRegionMat(recordRegions))
	{
		return false;
	}
	
	Mat intersectRegion;

	// 求当前绘制区域和之前记录的区域的交集
	bitwise_and(recordRegions, curRegionRecord, intersectRegion);
	//imwrite("intersectRegion.bmp", intersectRegion);

	vector < vector < cv::Point > > contourTmp;
	vector<Vec4i> hierarchy;
	// 找出交集轮廓
	findContours(intersectRegion, contourTmp, hierarchy, CV_RETR_LIST,
		CV_CHAIN_APPROX_SIMPLE);

	// 交集轮廓数不为0说明和之前绘制的区域存在交集，则当前用户绘制的区域无效
	if (contourTmp.size() > 0)
	{
		return false;
	}

	// 交集轮廓数不为0说明和之前绘制的区域不存在交集，将当前用户绘制区域加入
	// 区域的记录
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
// Description : 鼠标事件响应
// Parameters :  nEvent: 事件
//				 x：鼠标x坐标偏移量
//				 y：鼠标y坐标偏移量
// Return Value :true: 可以找到映射函数，成功
//				 false: 不能找到映射函数，失败
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
// Description : 重绘site检测区域
// Parameters :  dstMat: 绘制目标
// Return Value :true - site检测区域有效(已初始化)
//				 false - site检测区域无效
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
// Description : 重绘各个ROI区域
// Parameters :  dstMat: 绘制目标
// Return Value :无
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

	// 之前没有ROI区域记录，不用重绘
	if (contourTmp.size() == 0)
	{
		return;
	}

	drawContours(dstMat, contourTmp, -1, ROIColor);	
}

// ------------------------------------------------------------
// Description : 绘制圆的消息响应
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

		// 绘制圆心
		cv::circle(dispMat, m_anchorPoint, 1, ROIColor);
		// 绘制圆
		cv::circle(dispMat, m_anchorPoint, m_nROIRadius, ROIColor);
	}
	else if (CV_EVENT_LBUTTONUP == nEvent)
	{
		// 记录当前绘制的区域
		Mat curRegionRecord = Mat::zeros(m_srcImg->rows, m_srcImg->cols,
			CV_8UC1);
		// 将当前绘制区域保存在一个临时Mat里
		cv::circle(curRegionRecord, m_anchorPoint, m_nROIRadius,
			Scalar(m_curRecordIdx), -1);
		// 判断当前区域和之前绘制的ROI区域有无相交，如果有相交，将当前区域当成无效
		// 区域，不添加到ROI record 里
		AddNewParamToRecord(curRegionRecord);		
	}

	RedrawSiteRegion(dispMat);
	RedrawROI(dispMat);
	imshow(SHOW_WIN_NAME, dispMat);
}

// ------------------------------------------------------------
// Description : 绘制矩形的消息响应
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
		// 判断当前区域和之前绘制的ROI区域有无相交，如果有相交，将当前区域当成无效
		// 区域，不添加到ROI record 里
		if (!m_bDrawSiteRegion)
		{
			// 记录当前绘制的区域
			Mat curRegionRecord = Mat::zeros(m_srcImg->rows, m_srcImg->cols, CV_8UC1);
			// 将当前绘制区域保存在一个临时Mat里
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
// Description : 绘制自由曲线的消息响应
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
		// 记录当前绘制的区域
		Mat curRegionRecord = Mat::zeros(m_srcImg->rows, m_srcImg->cols, CV_8UC1);

		vector<vector<Point>> contour;
		contour.push_back(m_ROITracePointVec);

		// 将当前绘制区域保存在一个临时Mat里
		drawContours(curRegionRecord, contour, 0, Scalar(m_curRecordIdx), -1);

		// 判断当前区域和之前绘制的ROI区域有无相交，如果有相交，将当前区域当成无效
		// 区域，不添加到ROI record 里
		AddNewParamToRecord(curRegionRecord);
	}

	RedrawSiteRegion(dispMat);
	RedrawROI(dispMat);
	imshow(SHOW_WIN_NAME, dispMat);
}

// ------------------------------------------------------------
// Description : 保存参数到文件
// Parameters :  path: 参数保存路径
// Return Value :true - 成功
//				 false - 失败
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