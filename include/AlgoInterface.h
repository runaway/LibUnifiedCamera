#include "ComDef.h"
//#include "AlgoGoldenAnalysis.h"

#pragma once

// ------------------------------------------------------------
// Description: 初始化。
// Parameters : index: 线程序号，第一个线程是0，目前最多支持32线程
//				imageWidth: 图像宽度
//				imageHeight: 图像高度
//				fPixelSizeRatio: "像素-实际物理尺寸"的比例
//				configPath: 配置文件路径
//				outputPath: json数据和缺陷图片的保存路径；
//
// Return Value: INPUT_ERROR - 值为2，输入错误
//				 LOAD_CONFIG_FAIL - 值为3，加载配置文件失败
//				 ERROR_NONE - 值为3，成功
// ------------------------------------------------------------
extern "C" _declspec(dllexport) int AlgoInit(int index, int imageWidth, int imageHeight, float fPixelSizeRatio, const char* configPath, const char* outputPath);
// ------------------------------------------------------------
// Description:  退出清理资源。
// Parameters :  index: 线程序号，第一个线程是0，目前最多支持32线程
// Return Value: 无
// ------------------------------------------------------------
//extern "C" _declspec(dllexport) bool AlgoRelease(int index);

// ------------------------------------------------------------
// Description:  设置对正搜索范围，该范围对所有的site生效
// Parameters :  index: 线程序号
//				 alignOffX: 水平对正在[-alignOffX, alignOffX]
//				 alignOffY: 垂直对正在[-alignOffY, alignOffY]
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetAlignRange(int index, int alignOffX, int alignOffY);
// ------------------------------------------------------------
// Description:  设置用于对正的区间，针对当前的site设置
// Parameters :  index: 线程序号
//				 jsonRegion: json数据，包含了区间的数目及坐标
//				 nSiteColumnId: site 水平位置
//				 nSiteRowId: site 垂直位置
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetAlignRegion(int index, const char* jsonRegion, int nSiteColumnId, int nSiteRowId);
// ------------------------------------------------------------
// Description:  设置最小的对正分数，小于此值认为对正失败
// Parameters :  index: 线程序号
//				 alignScore: 最小对正分数
//
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetAlignMinScore(int index, float alignScore);

// ------------------------------------------------------------
// Description:  设置site检测区域
// Parameters :  index: 线程序号
// 				 siteRect 当前site检测区域
//				 param 当前site的检测参数
//				 nSiteColumnId: site 水平位置
//				 nSiteRowId: site 垂直位置
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetSiteRect(int index, Rect_t siteRect, Param_t param, ThreshGroup_t thresh, int nSiteColumnId, int nSiteRowId);

// ------------------------------------------------------------
// Description:  设置RoI参数
// Parameters :  index: 线程序号
// 				 jsonRoiSetting: json格式RoI参数
//				 nSiteColumnId: site 水平位置
//				 nSiteRowId: site 垂直位置
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetRoI(int index, const char* jsonRoiSetting, int nSiteColumnId, int nSiteRowId);

// ------------------------------------------------------------
// Description:  设置match方式
// Parameters :  index: 线程序号
//  			 mode: match方式
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetMatchMode(int index, MatchType mode);

// ------------------------------------------------------------
// Description:  设置ring区的宽度（需要在AlgoSetGoldenImage之前调用）
// Parameters :  index: 线程序号
//               nThres: 二值化阈值
//  			 nWidth: 宽度(如果设置为0，这不存在ring区)
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetRingWidth(int index,int nThres, int nWidth);

// ------------------------------------------------------------
// Description:  设置保存检测出缺陷的图像
// Parameters :  index: 线程序号
// 				 bSaveImage:是否保存图像
//				 bDrawDefects: 保存的图像上是否标记缺陷
// Return Value: true - 设置成功
//				 false - 设置失败
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetSaveDefectImage(int index, bool bSaveImage, bool bDrawDefects);

// Description : 创建金标准图像
// Parameters :  szGoldenImgPath:用于创建金标准图像的图像文件所在目录
//				 pbyBaseImg: 创建金标准图像的基准图像，如果为空指针，则用目录下的第一个图像文件作为基准图像
//				 pbyGoldenImg: 返回创建成功的金标准图像，指针不能为空
//				 unsigned char* goldenImage 返回生成的金标准图片
//				 alignOffX: match水平偏移范围[-alignOffX, alignOffX]
//				 alignOffY: match垂直偏移范围[-alignOffY, alignOffY]
//				 jsonRegion: 用于对正的ROI区域
// Return Value : true: 创建成功
//				  false: 创建失败，如果目录下文件数目少于9，则失败
// ------------------------------------------------------------
extern "C"  __declspec(dllexport) bool AlgoCreateGoldenImage(const char* szGoldenImgPath, unsigned char* pbyGoldenImg, unsigned char* pbyBaseImg, int alignOffX, int alignOffY,  const char* jsonRegion);

// ------------------------------------------------------------
// Description:  设置金标准图像
// Parameters :  pbyGoldenImg: 金标准图像数据指针，不能为空
//				 nSiteColumnId: site 水平位置
//				 nSiteRowId: site 垂直位置
// Return Value: true: 设置成功
//				 false: 设置失败
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetGoldenImage(int index, unsigned char* pbyGoldenImg, int nSiteColumnId, int nSiteRowId);

// ------------------------------------------------------------
// Description:  分析函数
// Parameters :  inputData: 待检数据的指针
//				 nDieId: die ID
//				 nSiteColumnId: site 水平位置
//				 nSiteRowId: site 垂直位置
// Return Value: ERROR_NONE - 值为3，函数执行成功
//				 INPUT_ERROR - 值为4，函数执行出错
// ------------------------------------------------------------
extern "C" _declspec(dllexport) int AlgoDoAnalysis(int index, unsigned char* imageData, int nDieId, int nSiteColumnId, int nSiteRowId);

// ------------------------------------------------------------
// Description:  分析函数(内部测试用)
// Parameters :  inputData: 待检数据的指针
//				 nDieId: die ID
//				 nSiteColumnId: site 水平位置
//				 nSiteRowId: site 垂直位置
// Return Value: INPUT_ERROR - 值为2，输入错误
//				 ERROR_NONE - 值为3，成功
// ------------------------------------------------------------
AlgorithmErrorType AlgoDoAnalysisInner(int index, unsigned char* imageData, int nDieId, int nSiteColumnId, int nSiteRowId, std::vector<defectDescription<float> >& outputResult, unsigned char* resultData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" _declspec(dllexport) bool AlgoSetClustering(int index, bool bEnableClustering, unsigned int nRadius);





