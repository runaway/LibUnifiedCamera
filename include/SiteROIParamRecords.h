#pragma once

#include "ComDef.h"
#include <map>
#include <opencv2/opencv.hpp>

// map<区域索引，对应参数>
typedef std::map<int, ROIParam> ROIParamMap;

class SiteROIParamRecords
{
public:
	SiteROIParamRecords(int nImgWid, int nImgHei);
	~SiteROIParamRecords();

	// ------------------------------------------------------------
	// Description : 设置site的区域
	// Parameters :  siteRect: site的区域
	// Return Value :true - siteRect参数有效
	//				 false - siteRect参数无效
	// ------------------------------------------------------------
	bool SetSiteRegion(const cv::Rect& siteRect);

	// ------------------------------------------------------------
	// Description : 向map中增加一个新的参数记录
	// Parameters :  nIdx: 记录索引
	//				 UserParam: 需增加的参数记录
	// Return Value :1: map中没有nIdx的记录，增加成功
	//				 0: map中有nIdx的记录，修改记录
	// ------------------------------------------------------------
	int InsertNewRecord(int nIdx, ROIParam param);

	// ------------------------------------------------------------
	// Description : 保存参数到xml文件
	// Parameters :  path: 参数保存路径
	// Return Value :true - 保存成功
	//				 false - 保存失败
	// ------------------------------------------------------------
	bool SaveRecordsToFile(char* path);

	// ------------------------------------------------------------
	// Description : 加载配置文件
	// Parameters :  filePath: 配置文件路径
	//				 nSiteID: site ID
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool LoadConfigFile(char* filePath, int nSiteID);

	// ------------------------------------------------------------
	// Description : 删除map中的一条记录
	// Parameters :  ind：待删除的记录的索引
	// Return Value :true: map中有nIdx的记录，删除成功
	//				 false: map中没有nIdx的记录，删除失败
	// ------------------------------------------------------------
	bool DeleteRecord(int nIdx);

	// ------------------------------------------------------------
	// Description : 获取ROI region Mat
	// Parameters :  无
	// Return Value :ROI region Mat
	// ------------------------------------------------------------
	bool GetROIRegionMat(cv::Mat& roiMat);

	// ------------------------------------------------------------
	// Description :  根据配置文件创建ROI
	// Parameters :   无
	// Return Value : true - 创建成功
	//				  false - 配置文件读取失败
	// ------------------------------------------------------------
	bool SiteROIParamRecords::CreateROIMat();
	

private:
	ROIParamMap m_paramMap;		// 记录ROI索引值和其对应参数的map
	cv::Mat m_ROIRegion;		// ROI region
	cv::Rect m_SiteRect;		// site的检测区域
	std::map<std::string, ROIShape> m_ROINameShapeMap;  // ROIShape枚举和ROI名称的map
};

