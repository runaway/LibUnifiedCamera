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
	float manualThreshold;				// 人工灰度阈值
	float minSize;						// 尺寸阈值
	float minArea;						// 面积阈值
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
	Point_t center;	//圆心坐标
	float   radius;	//半径
	Param_t param;	//当前圆ROI的检测参数
};
struct Polygon_t	//矩形作为多边形的一种，不单独处理
{
	int vertexNum;	//多边形的顶点数
	Point_t* vertexArray;	//多边形顶点坐标数组，按逆时针次序存放
	Param_t param;	//当前多边形ROI的检测参数
};

struct ROI_t
{
	int polyNum;	//多边形ROI数目，没有为0
	Polygon_t* polyArray;	//多边形ROI数组

	int circleNum;	//圆形ROI数目，没有为0
	Circle_t* circleArray;	//圆形ROI数组
};
#endif
// ------------------------------------------------------------
// Description : 检测当前目标机是否安装CUDA
// Parameters :  无
// Return Value :true - 有CUDA
//				 false - 无CUDA
// ------------------------------------------------------------
bool QueryCUDA();

// ------------------------------------------------------------
// Description : 缺陷“检测/分类/聚类”对外接口
// Parameters :  dstImgPath：待检图像路径
//               goldenImgPath：金标准图像文件夹路径
//			     configName: 使用的配置文件名称
//				 siteRect: site的检测区域	
//				 pixelSizeRatio: 每像素对应的物理尺寸
//				 jsonResult: 【输出值】json格式数值结果的字符串
//				 outputImgPath:【输出值】结果图像路径
//				 outputResult: 【输出值】stl容器保存的结果
//				 bWithRotate: 是否需要旋转匹配
// Return Value :true - 成功
//			     false - 失败
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
	std::vector<defectDescription<float> >& outputResult,		// 给软件的时候删掉
	cv::Mat& resultMat,									// 给软件的时候删掉
	bool bWithRotate);									// 给软件的时候删掉									// 给软件的时候删掉

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
// Description : 缺陷“检测/分类/聚类”对外接口,批处理接口
// Parameters :  dstImgPath：待检图像文件夹路径
//               goldenImgPath：金标准图像文件夹路径
//			     configName: 使用的配置文件名称
//				 siteRect: site的检测区域	
//				 pixelSizeRatio: 每像素对应的物理尺寸
//				 jsonResult: 【输出值】json格式数值结果的字符串
//				 nImgNum: 【输出值】处理图像数量
//				 outputResult: 【输出值】stl容器保存的结果
//				 resultMat: 【输出值】结果图像
//				 bWithRotate: 是否需要旋转匹配
// Return Value :true - 成功
//			     false - 失败
// ------------------------------------------------------------
bool DoBatchAnalysis(const std::string& dstImgPath,
	const std::string& goldenImgPath,
	const std::string& configName,
	const cv::Rect& siteRect,
	double pixelSizeRatio,
	std::string& jsonResult,
	std::string& outputImgPath,
	int& nImgNum,
	std::vector<defectDescription<float> >& outputResult,	// 给软件的时候删掉
	cv::Mat& resultMat,										// 给软件的时候删掉
	bool bParallel,											// 给软件的时候删掉			
	bool bWithRotate);										// 给软件的时候删掉	
															// 给软件的时候删掉
	
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

