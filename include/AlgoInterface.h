#include "ComDef.h"
//#include "AlgoGoldenAnalysis.h"

#pragma once

// ------------------------------------------------------------
// Description: ��ʼ����
// Parameters : index: �߳���ţ���һ���߳���0��Ŀǰ���֧��32�߳�
//				imageWidth: ͼ����
//				imageHeight: ͼ��߶�
//				fPixelSizeRatio: "����-ʵ������ߴ�"�ı���
//				configPath: �����ļ�·��
//				outputPath: json���ݺ�ȱ��ͼƬ�ı���·����
//
// Return Value: INPUT_ERROR - ֵΪ2���������
//				 LOAD_CONFIG_FAIL - ֵΪ3�����������ļ�ʧ��
//				 ERROR_NONE - ֵΪ3���ɹ�
// ------------------------------------------------------------
extern "C" _declspec(dllexport) int AlgoInit(int index, int imageWidth, int imageHeight, float fPixelSizeRatio, const char* configPath, const char* outputPath);
// ------------------------------------------------------------
// Description:  �˳�������Դ��
// Parameters :  index: �߳���ţ���һ���߳���0��Ŀǰ���֧��32�߳�
// Return Value: ��
// ------------------------------------------------------------
//extern "C" _declspec(dllexport) bool AlgoRelease(int index);

// ------------------------------------------------------------
// Description:  ���ö���������Χ���÷�Χ�����е�site��Ч
// Parameters :  index: �߳����
//				 alignOffX: ˮƽ������[-alignOffX, alignOffX]
//				 alignOffY: ��ֱ������[-alignOffY, alignOffY]
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetAlignRange(int index, int alignOffX, int alignOffY);
// ------------------------------------------------------------
// Description:  �������ڶ��������䣬��Ե�ǰ��site����
// Parameters :  index: �߳����
//				 jsonRegion: json���ݣ��������������Ŀ������
//				 nSiteColumnId: site ˮƽλ��
//				 nSiteRowId: site ��ֱλ��
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetAlignRegion(int index, const char* jsonRegion, int nSiteColumnId, int nSiteRowId);
// ------------------------------------------------------------
// Description:  ������С�Ķ���������С�ڴ�ֵ��Ϊ����ʧ��
// Parameters :  index: �߳����
//				 alignScore: ��С��������
//
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetAlignMinScore(int index, float alignScore);

// ------------------------------------------------------------
// Description:  ����site�������
// Parameters :  index: �߳����
// 				 siteRect ��ǰsite�������
//				 param ��ǰsite�ļ�����
//				 nSiteColumnId: site ˮƽλ��
//				 nSiteRowId: site ��ֱλ��
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetSiteRect(int index, Rect_t siteRect, Param_t param, ThreshGroup_t thresh, int nSiteColumnId, int nSiteRowId);

// ------------------------------------------------------------
// Description:  ����RoI����
// Parameters :  index: �߳����
// 				 jsonRoiSetting: json��ʽRoI����
//				 nSiteColumnId: site ˮƽλ��
//				 nSiteRowId: site ��ֱλ��
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetRoI(int index, const char* jsonRoiSetting, int nSiteColumnId, int nSiteRowId);

// ------------------------------------------------------------
// Description:  ����match��ʽ
// Parameters :  index: �߳����
//  			 mode: match��ʽ
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetMatchMode(int index, MatchType mode);

// ------------------------------------------------------------
// Description:  ����ring���Ŀ�ȣ���Ҫ��AlgoSetGoldenImage֮ǰ���ã�
// Parameters :  index: �߳����
//               nThres: ��ֵ����ֵ
//  			 nWidth: ���(�������Ϊ0���ⲻ����ring��)
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetRingWidth(int index,int nThres, int nWidth);

// ------------------------------------------------------------
// Description:  ���ñ������ȱ�ݵ�ͼ��
// Parameters :  index: �߳����
// 				 bSaveImage:�Ƿ񱣴�ͼ��
//				 bDrawDefects: �����ͼ�����Ƿ���ȱ��
// Return Value: true - ���óɹ�
//				 false - ����ʧ��
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetSaveDefectImage(int index, bool bSaveImage, bool bDrawDefects);

// Description : �������׼ͼ��
// Parameters :  szGoldenImgPath:���ڴ������׼ͼ���ͼ���ļ�����Ŀ¼
//				 pbyBaseImg: �������׼ͼ��Ļ�׼ͼ�����Ϊ��ָ�룬����Ŀ¼�µĵ�һ��ͼ���ļ���Ϊ��׼ͼ��
//				 pbyGoldenImg: ���ش����ɹ��Ľ��׼ͼ��ָ�벻��Ϊ��
//				 unsigned char* goldenImage �������ɵĽ��׼ͼƬ
//				 alignOffX: matchˮƽƫ�Ʒ�Χ[-alignOffX, alignOffX]
//				 alignOffY: match��ֱƫ�Ʒ�Χ[-alignOffY, alignOffY]
//				 jsonRegion: ���ڶ�����ROI����
// Return Value : true: �����ɹ�
//				  false: ����ʧ�ܣ����Ŀ¼���ļ���Ŀ����9����ʧ��
// ------------------------------------------------------------
extern "C"  __declspec(dllexport) bool AlgoCreateGoldenImage(const char* szGoldenImgPath, unsigned char* pbyGoldenImg, unsigned char* pbyBaseImg, int alignOffX, int alignOffY,  const char* jsonRegion);

// ------------------------------------------------------------
// Description:  ���ý��׼ͼ��
// Parameters :  pbyGoldenImg: ���׼ͼ������ָ�룬����Ϊ��
//				 nSiteColumnId: site ˮƽλ��
//				 nSiteRowId: site ��ֱλ��
// Return Value: true: ���óɹ�
//				 false: ����ʧ��
// ------------------------------------------------------------
extern "C" _declspec(dllexport) bool AlgoSetGoldenImage(int index, unsigned char* pbyGoldenImg, int nSiteColumnId, int nSiteRowId);

// ------------------------------------------------------------
// Description:  ��������
// Parameters :  inputData: �������ݵ�ָ��
//				 nDieId: die ID
//				 nSiteColumnId: site ˮƽλ��
//				 nSiteRowId: site ��ֱλ��
// Return Value: ERROR_NONE - ֵΪ3������ִ�гɹ�
//				 INPUT_ERROR - ֵΪ4������ִ�г���
// ------------------------------------------------------------
extern "C" _declspec(dllexport) int AlgoDoAnalysis(int index, unsigned char* imageData, int nDieId, int nSiteColumnId, int nSiteRowId);

// ------------------------------------------------------------
// Description:  ��������(�ڲ�������)
// Parameters :  inputData: �������ݵ�ָ��
//				 nDieId: die ID
//				 nSiteColumnId: site ˮƽλ��
//				 nSiteRowId: site ��ֱλ��
// Return Value: INPUT_ERROR - ֵΪ2���������
//				 ERROR_NONE - ֵΪ3���ɹ�
// ------------------------------------------------------------
AlgorithmErrorType AlgoDoAnalysisInner(int index, unsigned char* imageData, int nDieId, int nSiteColumnId, int nSiteRowId, std::vector<defectDescription<float> >& outputResult, unsigned char* resultData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" _declspec(dllexport) bool AlgoSetClustering(int index, bool bEnableClustering, unsigned int nRadius);





