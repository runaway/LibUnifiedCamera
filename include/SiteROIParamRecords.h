#pragma once

#include "ComDef.h"
#include <map>
#include <opencv2/opencv.hpp>

// map<������������Ӧ����>
typedef std::map<int, ROIParam> ROIParamMap;

class SiteROIParamRecords
{
public:
	SiteROIParamRecords(int nImgWid, int nImgHei);
	~SiteROIParamRecords();

	// ------------------------------------------------------------
	// Description : ����site������
	// Parameters :  siteRect: site������
	// Return Value :true - siteRect������Ч
	//				 false - siteRect������Ч
	// ------------------------------------------------------------
	bool SetSiteRegion(const cv::Rect& siteRect);

	// ------------------------------------------------------------
	// Description : ��map������һ���µĲ�����¼
	// Parameters :  nIdx: ��¼����
	//				 UserParam: �����ӵĲ�����¼
	// Return Value :1: map��û��nIdx�ļ�¼�����ӳɹ�
	//				 0: map����nIdx�ļ�¼���޸ļ�¼
	// ------------------------------------------------------------
	int InsertNewRecord(int nIdx, ROIParam param);

	// ------------------------------------------------------------
	// Description : ���������xml�ļ�
	// Parameters :  path: ��������·��
	// Return Value :true - ����ɹ�
	//				 false - ����ʧ��
	// ------------------------------------------------------------
	bool SaveRecordsToFile(char* path);

	// ------------------------------------------------------------
	// Description : ���������ļ�
	// Parameters :  filePath: �����ļ�·��
	//				 nSiteID: site ID
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool LoadConfigFile(char* filePath, int nSiteID);

	// ------------------------------------------------------------
	// Description : ɾ��map�е�һ����¼
	// Parameters :  ind����ɾ���ļ�¼������
	// Return Value :true: map����nIdx�ļ�¼��ɾ���ɹ�
	//				 false: map��û��nIdx�ļ�¼��ɾ��ʧ��
	// ------------------------------------------------------------
	bool DeleteRecord(int nIdx);

	// ------------------------------------------------------------
	// Description : ��ȡROI region Mat
	// Parameters :  ��
	// Return Value :ROI region Mat
	// ------------------------------------------------------------
	bool GetROIRegionMat(cv::Mat& roiMat);

	// ------------------------------------------------------------
	// Description :  ���������ļ�����ROI
	// Parameters :   ��
	// Return Value : true - �����ɹ�
	//				  false - �����ļ���ȡʧ��
	// ------------------------------------------------------------
	bool SiteROIParamRecords::CreateROIMat();
	

private:
	ROIParamMap m_paramMap;		// ��¼ROI����ֵ�����Ӧ������map
	cv::Mat m_ROIRegion;		// ROI region
	cv::Rect m_SiteRect;		// site�ļ������
	std::map<std::string, ROIShape> m_ROINameShapeMap;  // ROIShapeö�ٺ�ROI���Ƶ�map
};

