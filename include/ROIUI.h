#pragma once

#include <opencv2/opencv.hpp>
#include "ComDef.h"
#include "SiteROIParamRecords.h"


// ------------------------------------------------------------
// Description : ROI鼠标事件回调函数
// Parameters : 
// Return Value :
// ------------------------------------------------------------
void ROIMouseCallBack(int nEvent, int x, int y, int flags, void*param);


class ROIUI
{
	typedef void (ROIUI::*FuncPtr)(int nEvent, int x, int y); // 鼠标事件响应方法表驱动映射
	friend void ROIMouseCallBack(int nEvent, int x, int y, int flags, void*param);
public:
	ROIUI();
	~ROIUI();

	// ------------------------------------------------------------
	// Description : 绘制site的检测区域
	// Parameters :  srcImg：绘制的图像
	//				 nWid: 窗口宽度
	//				 nHei: 窗口高度
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool DrawSiteRegion(cv::Mat& srcImg, int nWid, int nHei);

	// ------------------------------------------------------------
	// Description : 获取site检测区域
	// Parameters :  无
	// Return Value :site检测区域
	// ------------------------------------------------------------
	cv::Rect GetSiteRect();

	// ------------------------------------------------------------
	// Description : 用户绘制ROI
	// Parameters :  srcImg: 绘制的图像
	//				 roiShape: roi形状
	//				 configName: ROI对应的配置文件的名称
	//				 nWid: 窗口宽度
	//				 nHei: 窗口高度
	// Return Value :成功
	// ------------------------------------------------------------
	bool DrawROI(cv::Mat& srcImg, ROIShape roiShape, std::string& configName,
		int nWid, int nHei);

	// ------------------------------------------------------------
	// Description : 保存参数到文件
	// Parameters :  path: 参数保存路径
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool SaveRecordToFile(char* path);

private:
	// ------------------------------------------------------------
	// Description : 对矩形ROI范围进行限制
	// Parameters :  ROIRect：矩形ROI范围
	//				 nSrcWid: 图像宽度
	//				 nSrcHei: 图像高度
	// Return Value :无
	// ------------------------------------------------------------
	void LimitROIRect(cv::Rect& ROIRect, int nSrcWid, int nSrcHei);

	// ------------------------------------------------------------
	// Description : 向当前记录表增加一条区域记录
	// Parameters :  curRegionRecord: 当前用户绘制的区域
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool AddNewParamToRecord(cv::Mat& curRegionRecord);

	// ------------------------------------------------------------
	// Description : 重绘site检测区域
	// Parameters :  dstMat: 绘制目标
	// Return Value :true - site检测区域有效(已初始化)
	//				 false - site检测区域无效
	// ------------------------------------------------------------
	bool RedrawSiteRegion(cv::Mat& dstMat);

	// ------------------------------------------------------------
	// Description : 重绘各个ROI区域
	// Parameters :  dstMat: 绘制目标
	// Return Value :无
	// ------------------------------------------------------------
	void RedrawROI(cv::Mat& dstMat);

	// ------------------------------------------------------------
	// Description : 鼠标事件响应
	// Parameters :  nEvent: 事件
	//				 x：鼠标x坐标偏移量
	//				 y：鼠标y坐标偏移量
	// Return Value :true: 可以找到映射函数，成功
	//				 false: 不能找到映射函数，失败
	// ------------------------------------------------------------
	bool ResponseROIDrawing(int nEvent, int x, int y);

	// ------------------------------------------------------------
	// Description : 绘制圆的消息响应
	// Parameters : nEvent: 事件
	//				 x：鼠标x坐标偏移量
	//				 y：鼠标y坐标偏移量
	// Return Value :无
	// ------------------------------------------------------------
	void ResponseDrawCircle(int nEvent, int x, int y);

	// ------------------------------------------------------------
	// Description : 绘制矩形的消息响应
	// Parameters :  nEvent: 事件
	//				 x：鼠标x坐标偏移量
	//				 y：鼠标y坐标偏移量
	// Return Value :无
	// ------------------------------------------------------------
	void ResponseDrawRectangle(int nEvent, int x, int y);

	// ------------------------------------------------------------
	// Description : 绘制自由曲线的消息响应
	// Parameters :  nEvent: 事件
	//				 x：鼠标x坐标偏移量
	//				 y：鼠标y坐标偏移量
	// Return Value :无
	// ------------------------------------------------------------
	void ResponseDrawFreehandLine(int nEvent, int x, int y);
	
private:	
	SiteROIParamRecords* m_pROIParamRecords;
	cv::Mat* m_srcImg;
	ROIShape m_ROIShape;		// ROI形状	
	cv::Point m_anchorPoint;	// 圆中心点坐标
	unsigned int m_nROIRadius;	// 仅roiShape = ROI_RECTANGLE时有用，为圆半径
	cv::Rect m_ROIRect;			// 仅roiShape = ROI_RECTANGLE时有用，矩形的高度	
	cv::Rect m_SiteRect;		// site的检测区域
	bool m_bDrawSiteRegion;		// 记录当前是画site的区域还是ROI
	float m_fWidRatio;			// 窗口宽度和实际图像宽度的比例
	float m_fHeiRatio;			// 窗口高度和实际图像高度的比例
	std::vector<cv::Point> m_ROITracePointVec;	// 仅roiShape = ROI_FREEHAND_LINE时有用，
												// 为自由曲线的点列
	int m_curRecordIdx;							// 当前绘制的区域的编号		
	std::string m_configName;					// ROI区域对应的配置文件的名称
	std::map<ROIShape, FuncPtr> m_methodMap;	// 鼠标事件响应方法表驱动映射
};

