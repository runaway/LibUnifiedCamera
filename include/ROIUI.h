#pragma once

#include <opencv2/opencv.hpp>
#include "ComDef.h"
#include "SiteROIParamRecords.h"


// ------------------------------------------------------------
// Description : ROI����¼��ص�����
// Parameters : 
// Return Value :
// ------------------------------------------------------------
void ROIMouseCallBack(int nEvent, int x, int y, int flags, void*param);


class ROIUI
{
	typedef void (ROIUI::*FuncPtr)(int nEvent, int x, int y); // ����¼���Ӧ����������ӳ��
	friend void ROIMouseCallBack(int nEvent, int x, int y, int flags, void*param);
public:
	ROIUI();
	~ROIUI();

	// ------------------------------------------------------------
	// Description : ����site�ļ������
	// Parameters :  srcImg�����Ƶ�ͼ��
	//				 nWid: ���ڿ��
	//				 nHei: ���ڸ߶�
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool DrawSiteRegion(cv::Mat& srcImg, int nWid, int nHei);

	// ------------------------------------------------------------
	// Description : ��ȡsite�������
	// Parameters :  ��
	// Return Value :site�������
	// ------------------------------------------------------------
	cv::Rect GetSiteRect();

	// ------------------------------------------------------------
	// Description : �û�����ROI
	// Parameters :  srcImg: ���Ƶ�ͼ��
	//				 roiShape: roi��״
	//				 configName: ROI��Ӧ�������ļ�������
	//				 nWid: ���ڿ��
	//				 nHei: ���ڸ߶�
	// Return Value :�ɹ�
	// ------------------------------------------------------------
	bool DrawROI(cv::Mat& srcImg, ROIShape roiShape, std::string& configName,
		int nWid, int nHei);

	// ------------------------------------------------------------
	// Description : ����������ļ�
	// Parameters :  path: ��������·��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool SaveRecordToFile(char* path);

private:
	// ------------------------------------------------------------
	// Description : �Ծ���ROI��Χ��������
	// Parameters :  ROIRect������ROI��Χ
	//				 nSrcWid: ͼ����
	//				 nSrcHei: ͼ��߶�
	// Return Value :��
	// ------------------------------------------------------------
	void LimitROIRect(cv::Rect& ROIRect, int nSrcWid, int nSrcHei);

	// ------------------------------------------------------------
	// Description : ��ǰ��¼������һ�������¼
	// Parameters :  curRegionRecord: ��ǰ�û����Ƶ�����
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool AddNewParamToRecord(cv::Mat& curRegionRecord);

	// ------------------------------------------------------------
	// Description : �ػ�site�������
	// Parameters :  dstMat: ����Ŀ��
	// Return Value :true - site���������Ч(�ѳ�ʼ��)
	//				 false - site���������Ч
	// ------------------------------------------------------------
	bool RedrawSiteRegion(cv::Mat& dstMat);

	// ------------------------------------------------------------
	// Description : �ػ����ROI����
	// Parameters :  dstMat: ����Ŀ��
	// Return Value :��
	// ------------------------------------------------------------
	void RedrawROI(cv::Mat& dstMat);

	// ------------------------------------------------------------
	// Description : ����¼���Ӧ
	// Parameters :  nEvent: �¼�
	//				 x�����x����ƫ����
	//				 y�����y����ƫ����
	// Return Value :true: �����ҵ�ӳ�亯�����ɹ�
	//				 false: �����ҵ�ӳ�亯����ʧ��
	// ------------------------------------------------------------
	bool ResponseROIDrawing(int nEvent, int x, int y);

	// ------------------------------------------------------------
	// Description : ����Բ����Ϣ��Ӧ
	// Parameters : nEvent: �¼�
	//				 x�����x����ƫ����
	//				 y�����y����ƫ����
	// Return Value :��
	// ------------------------------------------------------------
	void ResponseDrawCircle(int nEvent, int x, int y);

	// ------------------------------------------------------------
	// Description : ���ƾ��ε���Ϣ��Ӧ
	// Parameters :  nEvent: �¼�
	//				 x�����x����ƫ����
	//				 y�����y����ƫ����
	// Return Value :��
	// ------------------------------------------------------------
	void ResponseDrawRectangle(int nEvent, int x, int y);

	// ------------------------------------------------------------
	// Description : �����������ߵ���Ϣ��Ӧ
	// Parameters :  nEvent: �¼�
	//				 x�����x����ƫ����
	//				 y�����y����ƫ����
	// Return Value :��
	// ------------------------------------------------------------
	void ResponseDrawFreehandLine(int nEvent, int x, int y);
	
private:	
	SiteROIParamRecords* m_pROIParamRecords;
	cv::Mat* m_srcImg;
	ROIShape m_ROIShape;		// ROI��״	
	cv::Point m_anchorPoint;	// Բ���ĵ�����
	unsigned int m_nROIRadius;	// ��roiShape = ROI_RECTANGLEʱ���ã�ΪԲ�뾶
	cv::Rect m_ROIRect;			// ��roiShape = ROI_RECTANGLEʱ���ã����εĸ߶�	
	cv::Rect m_SiteRect;		// site�ļ������
	bool m_bDrawSiteRegion;		// ��¼��ǰ�ǻ�site��������ROI
	float m_fWidRatio;			// ���ڿ�Ⱥ�ʵ��ͼ���ȵı���
	float m_fHeiRatio;			// ���ڸ߶Ⱥ�ʵ��ͼ��߶ȵı���
	std::vector<cv::Point> m_ROITracePointVec;	// ��roiShape = ROI_FREEHAND_LINEʱ���ã�
												// Ϊ�������ߵĵ���
	int m_curRecordIdx;							// ��ǰ���Ƶ�����ı��		
	std::string m_configName;					// ROI�����Ӧ�������ļ�������
	std::map<ROIShape, FuncPtr> m_methodMap;	// ����¼���Ӧ����������ӳ��
};

