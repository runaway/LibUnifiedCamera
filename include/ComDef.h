#pragma once

#include <string>
#include <opencv2/opencv.hpp>

#define SAVE_RESULT_TO_FILE 1		// 是否保存结果图像和json数据文件
#define CONFIGURATION_FILE_PATH ".\\configuration\\"
#define STRATEGY_FILE_PATH ".\\strategy\\"
#define SHOW_WIN_NAME "show win"

using namespace cv;
using namespace std;


// 目标机是否有CUDA
extern bool gHaveCUDA;

// 是否已检测过目标机CUDA的设置
extern bool gHaveQueryCUDA;

// 对准方式
enum MatchType {
	FASTEST = 0,                 //最快的投影对准
	GENERAL,                     //一般的四角对准
	MOSTACCURATE                 //最准的全图对准
};

// 阈值类型
enum thresholdType
{
	MANUAL = 0,		// 人工
	AUTO			// 自动
};

// 自动阈值类型
enum autoThresholdType
{
	HIGH = 0,
	MEDIUM,
	LOW

};

// 聚类方式
enum clusterType
{
	CLUSTER_NONE = -1,			// no Report Clusters as a Single Defect
	CLUSTER_ROI = 0,			// Report Clusters as a Single Defect with ROI
	CLUSTER_BINNING_STRATEGY	// Report Clusters as a Single Defect with strategy
};

enum AlgorithmErrorType
{
	HAVE_DEFECT_IN_PHOTOSENSITIVE_AREA = 0,				// 成功。在感光区域检测到缺陷
	HAVE_DEFECT_IN_NON_PHOTOSENSITIVE_AREA_SIZE = 1,	// 成功。在非感光区域检测到缺陷
	NO_DEFECT = 2,					// 成功。没有检测到缺陷	
	ERROR_NONE = 3,					// 成功
	INPUT_ERROR = 4,				// 输入错误
	LOAD_CONFIG_FAIL = 5,			// 加载配置文件失败
	DID_NOT_LOAD_CONFIG_FILE = 6,	// 没有加载配置文件
	NO_DIE = 7,						// 没有die区域
	DETECTION_FAIL = 8,				// 检测失败
	REMOVEIRRELEVANTREGIONS_FAIL = 9, // 根据配置文件参数去除无关区域失败
	DOCLASSIFY_FAIL = 10,			// 分类失败
	NO_INITED = 11,					//没有初始化
	SITE_OUTSIDE=12,				//SiteRect超出图像范围
	ALIGN_FAIL = 13					//对正失败
};

enum LowpassSize
{ 
	PHOTOSENSITIVE_AREA_SIZE = 91,
	NON_PHOTOSENSITIVE_AREA_SIZE = 91
};

// 检测类型
enum DetectionType
{
	BLOB_DETECTION = 0,		// 亮点黑点检测
	SCRATCH_DETECTION = 1,	// 刮痕检测
	LF_BLOB_DETECTION = 2,	// 明场斑点检测
	LIU_METHOD = 3,			// 明场小斑点的检测方法
	HUANG_METHOD			// 明场小斑点的检测方法
};


template<typename T>
struct DefectParam
{
	T centerX;
	T centerY;
	T area;
	T width;
	T length;
	float angle;
	float aspectRatio;
	unsigned char brightness;
};

// 缺陷结果参数
template<typename T>
struct defectDescription
{
	std::string strategyName;
	std::string binName;
	std::string className;
	int clusterID;
	DefectParam<T> defectData;
};

// ROI形状
enum ROIShape
{
	ROI_RECTANGLE = 0,
	ROI_CIRCLE,
	ROI_FREEHAND_LINE
};

// ROI参数
struct ROIParam 
{
	struct ROICircleParam
	{
		int x;
		int y;
		int radius;
	};

	struct ParamData
	{
		cv::Rect rect;
		ROICircleParam circle;
		std::vector<cv::Point> contour;
	};

	ROIShape shape;
	ParamData data;
	std::string configName;
};

struct Param_t
{
	//float manualThreshold;				// 人工灰度阈值
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
	bool IsInclude;
	Point_t center;	//圆心坐标
	float   radius;	//半径
	Param_t param;	//当前圆ROI的检测参数
};
struct Polygon_t	//矩形作为多边形的一种，不单独处理
{
	bool IsInclude;
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

struct URect
{
	double x;
	double y;
	double width;
	double height;
};

struct ThreshGroup_t
{//所有值都在[0,255]之间
	int highGray;				//高灰度值
	int lowGray;				//低灰度值, 必须满足lowGray<=highGray
	int highThreshAbove;		//高灰度阈值: 灰度值高于GoldenImage超过此值，认为是缺陷
	int highThreshBelow;		//高灰度阈值: 灰度值低于GoldenImage超过此值，认为是缺陷
	int lowThreshAbove;			//低灰度阈值: 灰度值高于GoldenImage超过此值，认为是缺陷
	int lowThreshBelow;			//低灰度阈值: 灰度值低于GoldenImage超过此值，认为是缺陷
};
struct RoiInfo_t
{
	bool isInclude;	//当前ROI是否检测
	Rect bound;		//当前ROI的外界矩形
	Param_t param;	//当前ROI的检测参数
	ThreshGroup_t thresh;	//当前ROI的阈值参数集
	Mat mask;		//当前ROI的有效区域mask
};
struct Rect_t
{
	int x;	//矩形左上角顶点x坐标
	int y;	//矩形左上角顶点y坐标
	int width;
	int height;
};

