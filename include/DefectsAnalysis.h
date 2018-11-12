#pragma once
#include <opencv2/opencv.hpp>
#include <string>
#include "ComDef.h"
#include <vector>
#include "ConfigurationManage.h"

#define IMAGE_WIDTH 1088
#define IMAGE_HEIGHT 2048

struct URectangle
{
    double x;
    double y;
    double width;
    double height;
    float manualThreshold;
    float minSize;
    float minArea;
    float minWidth;
    float minLength;
    float maxWidth;
    float maxLength;
};

struct AlignmentRange
{
	int iMarginH;
	int iMarginV;
};

#if 0// moved to comdef.h, 2017.Dec.04
struct URect
{
	double x;
	double y;
	double width;
	double height;
};

struct Param_t
{
	float manualThreshold;				// �˹��Ҷ���ֵ
	float minSize;						// �ߴ���ֵ
	float minArea;						// �����ֵ
	float minWidth;
	float minLength;
	float maxWidth;
	float maxLength;
};
struct Point_t
{
	float x;
	float y;
};

struct Circle_t
{
	Point_t center;	//Բ������
	float   radius;	//�뾶
	Param_t param;	//��ǰԲROI�ļ�����
};
struct Polygon_t	//������Ϊ����ε�һ�֣�����������
{
	int vertexNum;	//����εĶ�����
	Point_t* vertexArray;	//����ζ����������飬����ʱ�������
	Param_t param;	//��ǰ�����ROI�ļ�����
};

struct ROI_t
{
	int polyNum;	//�����ROI��Ŀ��û��Ϊ0
	Polygon_t* polyArray;	//�����ROI����

	int circleNum;	//Բ��ROI��Ŀ��û��Ϊ0
	Circle_t* circleArray;	//Բ��ROI����
};
#endif
// ------------------------------------------------------------
// Description : ��⵱ǰĿ����Ƿ�װCUDA
// Parameters :  ��
// Return Value :true - ��CUDA
//				 false - ��CUDA
// ------------------------------------------------------------
bool QueryCUDA();

// ------------------------------------------------------------
// Description : ȱ�ݡ����/����/���ࡱ����ӿ�
// Parameters :  dstImgPath������ͼ��·��
//               goldenImgPath�����׼ͼ���ļ���·��
//			     configName: ʹ�õ������ļ�����
//				 siteRect: site�ļ������	
//				 pixelSizeRatio: ÿ���ض�Ӧ������ߴ�
//				 jsonResult: �����ֵ��json��ʽ��ֵ������ַ���
//				 outputImgPath:�����ֵ�����ͼ��·��
//				 outputResult: �����ֵ��stl��������Ľ��
//				 bWithRotate: �Ƿ���Ҫ��תƥ��
// Return Value :true - �ɹ�
//			     false - ʧ��
// ------------------------------------------------------------

//__declspec(dllexport)
bool DoAnalysisInner(const std::string& dstImgPath,
	const std::string& goldenImgPath,
	const std::string& configName,
	cv::Rect siteRect,
	const ROI_t& roiRegion,
	double pixelSizeRatio,
	std::string& jsonResult,
	std::string& outputImgPath,
	std::vector<defectDescription<float> >& outputResult,		// �������ʱ��ɾ��
	cv::Mat& resultMat,									// �������ʱ��ɾ��
	bool bWithRotate);									// �������ʱ��ɾ��									// �������ʱ��ɾ��

//extern "C"  
//__declspec(dllexport)
bool CallDoAnalysis(
	unsigned char* pbyDstImgData,
	unsigned char* pbyGoldenImgData,
	char*  configName,
	URect siteRect,
	double pixelSizeRatio,
	char* jsonResult,
	char* outputImgPath,
	char* szDieID,
	char* szSiteID);

extern "C"  __declspec(dllexport)
int DoAnalysis(
	unsigned char* pbyDstImgData,
	unsigned char* pbyGoldenImgData,
	char*  configName,
	URectangle urectSite,
	char* szROIRegion,
	char* szROINegRegion,
	double pixelSizeRatio,
	char* jsonResult,
	char* outputImgPath,
	char* szDieID,
	char* szSiteID,
	MatchType matchtype = FASTEST);

extern "C"  __declspec(dllexport)
bool 
CreateGoldenImage(
    char* szGoldenImgPath, 
    unsigned char* pbyGoldenImg,
    unsigned char* pbyBaseImg,
    AlignmentRange sAlignmentRange);

// ------------------------------------------------------------
// Description : ȱ�ݡ����/����/���ࡱ����ӿ�,������ӿ�
// Parameters :  dstImgPath������ͼ���ļ���·��
//               goldenImgPath�����׼ͼ���ļ���·��
//			     configName: ʹ�õ������ļ�����
//				 siteRect: site�ļ������	
//				 pixelSizeRatio: ÿ���ض�Ӧ������ߴ�
//				 jsonResult: �����ֵ��json��ʽ��ֵ������ַ���
//				 nImgNum: �����ֵ������ͼ������
//				 outputResult: �����ֵ��stl��������Ľ��
//				 resultMat: �����ֵ�����ͼ��
//				 bWithRotate: �Ƿ���Ҫ��תƥ��
// Return Value :true - �ɹ�
//			     false - ʧ��
// ------------------------------------------------------------
bool DoBatchAnalysis(const std::string& dstImgPath,
	const std::string& goldenImgPath,
	const std::string& configName,
	const cv::Rect& siteRect,
	double pixelSizeRatio,
	std::string& jsonResult,
	std::string& outputImgPath,
	int& nImgNum,
	std::vector<defectDescription<float> >& outputResult,	// �������ʱ��ɾ��
	cv::Mat& resultMat,										// �������ʱ��ɾ��
	bool bParallel,											// �������ʱ��ɾ��			
	bool bWithRotate);										// �������ʱ��ɾ��	
															// �������ʱ��ɾ��
	
__declspec(dllexport)
bool DoBatchAnalysis(const std::string& dstImgPath,
	const std::string& goldenImgPath,
	const std::string& configName,
	const cv::Rect& siteRect,
	double pixelSizeRatio,
	std::string& jsonResult,
	std::string& outputImgPath,
	int& nImgNum,
	AlignmentRange sAlignmentRange);										
	


bool OneBatchProc(std::vector<std::string> &imgNameVec,
	const std::string& dstImgPath,
	const cv::Mat& goldenImg,
	unsigned char grayThreshold,
	const cv::Rect& siteRect,
	bool bParallel,
	bool bWithRotate,
	ConfigurationManage &configManage,
	double pixelSizeRatio,
	cv::Mat& resultMat,
	std::vector<defectDescription<float> >& outputResult,
	int& nDefectRegionSize);

