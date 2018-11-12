#pragma once

#include <string>
#include <opencv2/opencv.hpp>

#define SAVE_RESULT_TO_FILE 1		// �Ƿ񱣴���ͼ���json�����ļ�
#define CONFIGURATION_FILE_PATH ".\\configuration\\"
#define STRATEGY_FILE_PATH ".\\strategy\\"
#define SHOW_WIN_NAME "show win"

using namespace cv;
using namespace std;


// Ŀ����Ƿ���CUDA
extern bool gHaveCUDA;

// �Ƿ��Ѽ���Ŀ���CUDA������
extern bool gHaveQueryCUDA;

// ��׼��ʽ
enum MatchType {
	FASTEST = 0,                 //����ͶӰ��׼
	GENERAL,                     //һ����ĽǶ�׼
	MOSTACCURATE                 //��׼��ȫͼ��׼
};

// ��ֵ����
enum thresholdType
{
	MANUAL = 0,		// �˹�
	AUTO			// �Զ�
};

// �Զ���ֵ����
enum autoThresholdType
{
	HIGH = 0,
	MEDIUM,
	LOW

};

// ���෽ʽ
enum clusterType
{
	CLUSTER_NONE = -1,			// no Report Clusters as a Single Defect
	CLUSTER_ROI = 0,			// Report Clusters as a Single Defect with ROI
	CLUSTER_BINNING_STRATEGY	// Report Clusters as a Single Defect with strategy
};

enum AlgorithmErrorType
{
	HAVE_DEFECT_IN_PHOTOSENSITIVE_AREA = 0,				// �ɹ����ڸй������⵽ȱ��
	HAVE_DEFECT_IN_NON_PHOTOSENSITIVE_AREA_SIZE = 1,	// �ɹ����ڷǸй������⵽ȱ��
	NO_DEFECT = 2,					// �ɹ���û�м�⵽ȱ��	
	ERROR_NONE = 3,					// �ɹ�
	INPUT_ERROR = 4,				// �������
	LOAD_CONFIG_FAIL = 5,			// ���������ļ�ʧ��
	DID_NOT_LOAD_CONFIG_FILE = 6,	// û�м��������ļ�
	NO_DIE = 7,						// û��die����
	DETECTION_FAIL = 8,				// ���ʧ��
	REMOVEIRRELEVANTREGIONS_FAIL = 9, // ���������ļ�����ȥ���޹�����ʧ��
	DOCLASSIFY_FAIL = 10,			// ����ʧ��
	NO_INITED = 11,					//û�г�ʼ��
	SITE_OUTSIDE=12,				//SiteRect����ͼ��Χ
	ALIGN_FAIL = 13					//����ʧ��
};

enum LowpassSize
{ 
	PHOTOSENSITIVE_AREA_SIZE = 91,
	NON_PHOTOSENSITIVE_AREA_SIZE = 91
};

// �������
enum DetectionType
{
	BLOB_DETECTION = 0,		// ����ڵ���
	SCRATCH_DETECTION = 1,	// �κۼ��
	LF_BLOB_DETECTION = 2,	// �����ߵ���
	LIU_METHOD = 3,			// ����С�ߵ�ļ�ⷽ��
	HUANG_METHOD			// ����С�ߵ�ļ�ⷽ��
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

// ȱ�ݽ������
template<typename T>
struct defectDescription
{
	std::string strategyName;
	std::string binName;
	std::string className;
	int clusterID;
	DefectParam<T> defectData;
};

// ROI��״
enum ROIShape
{
	ROI_RECTANGLE = 0,
	ROI_CIRCLE,
	ROI_FREEHAND_LINE
};

// ROI����
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
	//float manualThreshold;				// �˹��Ҷ���ֵ
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
	bool IsInclude;
	Point_t center;	//Բ������
	float   radius;	//�뾶
	Param_t param;	//��ǰԲROI�ļ�����
};
struct Polygon_t	//������Ϊ����ε�һ�֣�����������
{
	bool IsInclude;
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

struct URect
{
	double x;
	double y;
	double width;
	double height;
};

struct ThreshGroup_t
{//����ֵ����[0,255]֮��
	int highGray;				//�߻Ҷ�ֵ
	int lowGray;				//�ͻҶ�ֵ, ��������lowGray<=highGray
	int highThreshAbove;		//�߻Ҷ���ֵ: �Ҷ�ֵ����GoldenImage������ֵ����Ϊ��ȱ��
	int highThreshBelow;		//�߻Ҷ���ֵ: �Ҷ�ֵ����GoldenImage������ֵ����Ϊ��ȱ��
	int lowThreshAbove;			//�ͻҶ���ֵ: �Ҷ�ֵ����GoldenImage������ֵ����Ϊ��ȱ��
	int lowThreshBelow;			//�ͻҶ���ֵ: �Ҷ�ֵ����GoldenImage������ֵ����Ϊ��ȱ��
};
struct RoiInfo_t
{
	bool isInclude;	//��ǰROI�Ƿ���
	Rect bound;		//��ǰROI��������
	Param_t param;	//��ǰROI�ļ�����
	ThreshGroup_t thresh;	//��ǰROI����ֵ������
	Mat mask;		//��ǰROI����Ч����mask
};
struct Rect_t
{
	int x;	//�������ϽǶ���x����
	int y;	//�������ϽǶ���y����
	int width;
	int height;
};

