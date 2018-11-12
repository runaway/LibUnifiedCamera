#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <windows.h>
#include "ComDef.h"
#include "DefectsAnalysis.h"

class DefectDetection
{
public:	
	
	// ------------------------------------------------------------
	// Description : ȱ�ݼ��
	// Parameters :  dstImg: �����ͼƬ
	//				 goldenImgPath: �������׼ͼƬ��·��
	//				 defectRegions: ������������ͼƬ����׼ͼƬ������
	//				 ͼƬ
	//				 bWithRotate: �Ƿ���Ҫ��תƥ��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool Detection(
		cv::Mat& dstImg, 
		const cv::Mat& goldenImg, 
		const cv::Mat& maskImg,
		cv::Rect siteRect,
		unsigned char thresholdVal,
		cv::Mat& defectRegionsFg,
		cv::Mat& defectRegionsBg,
		cv::Mat& diffMat,
		bool bWithRotate,
		MatchType matchtype = FASTEST);

	// ------------------------------------------------------------
	// Description : ������ͼ����׼�����
	// Parameters :  dstAImgVec - Ŀ��ͼƬ
	//				 goldenImg - ���׼ͼƬ
	//				 DiffRegionsVec - ��׼������ͼƬ
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool GetBatchDiffRegions(
		std::vector<cv::Mat>& dstImgVec,
		cv::Rect siteRect,
		const cv::Mat& goldenImg,
		std::vector<cv::Mat>& DiffRegionsVec);

	// ------------------------------------------------------------
	// Description : ȱ�ݼ��(��������)
	// Parameters :  dstImgPath: �����ͼƬ�ļ���·��
	//				 goldenImgPath: ���׼ͼƬ�ļ���·��
	//				 thresholdVal: �Ҷ���ֵ
	//				 siteRect: site�ļ������	
	//				 defectRegionsVec: �����ͼƬ����׼ͼƬ������
	//				 ͼƬ
	//				 bParallel: cpu�汾�Ƿ�ʹ�ò��м���
	//				 bWithRotate: �Ƿ���Ҫ��תƥ��
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool DefectDetection::Detection(
		std::vector<std::string>& imgNameVec,
		const std::string& dstImgPath,
		const cv::Mat& goldenImg, 
		unsigned char thresholdVal,
		cv::Rect siteRect,
		std::vector<cv::Mat>& defectRegionsVec,
		bool bParallel,
		bool bWithRotate);

	// ------------------------------------------------------------
	// Description : ����golden image
	// Parameters :  goldenImgPath: �������׼ͼƬ��·��
	//				 goldenImg: ���׼ͼƬ
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool CreateGoldenImage(
		const std::string& goldenImgPath,
		cv::Mat& goldenImg);

	bool CreateGoldenImage(
		const std::string& strGoldenImgPath,
		cv::Mat& matGoldenImg,
		cv::Mat matBaseImg,
		AlignmentRange sAlignmentRange);

	bool CreateGoldenMaskImage(
		const cv::Mat& goldenImg,
		cv::Mat& goldenMaskImg);

	bool DefectDetection::CreateGoldenSite(
		std::vector<cv::Mat>& vecmatDstImg,
		cv::Rect rectSite,
		cv::Mat matBaseGoldenImg,
		cv::Mat& matGoldenImg,
		AlignmentRange sAlignmentRange);

private:
	// ------------------------------------------------------------
	// Description : ͼ����׼�����
	// Parameters :  dstImg - Ŀ��ͼƬ
	//				 goldenImg - ���׼ͼƬ
	//				 DiffRegions - ��׼������ͼƬ
	// Return Value :true - �ɹ�
	//				 false - ʧ��
	// ------------------------------------------------------------
	bool GetDiffRegions(
		cv::Mat& dstImg,
		cv::Rect siteRect,
		const cv::Mat& goldenImg,
		cv::Mat& DiffRegions,
		int& iShiftX,
		int& iShiftY);

	bool GetDiffRegions(
		cv::Mat& dstImg,
		cv::Rect siteRect,
		const cv::Mat& goldenImg,
		cv::Mat& DiffRegions,
		int& iShiftX,
		int& iShiftY,
		cv::Mat matBaseGoldenImg);

	bool DefectDetection::GetDiffRegions(
		cv::Mat& dstImg,
		cv::Rect siteRect,
		const cv::Mat& goldenImg,
		const cv::Mat& maskImg,
		cv::Mat& DiffRegionsFg,
		cv::Mat& DiffRegionsBg,
		cv::Mat& DiffRegions,
		int& iShiftX,
		int& iShiftY,
		MatchType matchtype = FASTEST);

	bool MakeGoldenSite(
		std::vector<cv::Mat>& vecmatDstImg,
		cv::Rect rectSite,
		cv::Mat matBaseGoldenImg,
		cv::Mat& matGoldenImg);

	void Translate(cv::Mat const& matSrc, cv::Mat& matDst, int iShiftX, int iShiftY);

	BOOL Match(const cv::Mat Temp, const cv::Mat Samp, int &dx, int &dy, MatchType matchtype = FASTEST);
	BOOL GrayProjections(const cv::Mat src, cv::Rect rc, std::vector<double> &HorPro, std::vector<double> &VerPro, int &HorProMean);
	BOOL VectorMatch(std::vector<double> vt1, std::vector<double> vt2, int off, int &ndelta);
	BOOL SpliteVectorMatch(std::vector<double> vt1, std::vector<double> vt2, int off, int &ndelta);

	std::vector<double> VtSub(std::vector<double> samp, std::vector<double> temp);
	double VtDev(std::vector<double> vt);
	double VtMean(std::vector<double> vt);
	std::vector<double> VtSmooth(std::vector<double> vt, int kerWid);

	double VtRelativity(std::vector<double> samp, std::vector<double> temp);
};

