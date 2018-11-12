#include "stdafx.h"
#include "AlgoGoldenAnalysis.h"
#include "AlgoInterface.h"


static AlgoGoldenAnalysis algoInstance[MAX_THREAD_NUM];


int AlgoInit(int index, int imageWidth, int imageHeight, float fPixelSizeRatio, const char* configPath, const char* outputPath)
{
	if (NULL == configPath)	
		return INPUT_ERROR;

	if((index<0) || (index>=MAX_THREAD_NUM))
		return INPUT_ERROR;

	if(!algoInstance[index].Init(index, imageWidth, imageHeight, (float)fPixelSizeRatio, configPath, outputPath))
	{
		return LOAD_CONFIG_FAIL;
	}

	return ERROR_NONE;
}
bool AlgoSetAlignRange(int index, int alignOffX, int alignOffY)
{
	if((index<0) || (index>=MAX_THREAD_NUM))
		return false;
	
	algoInstance[index].SetAlignRange(alignOffX, alignOffY);

	return true;
}
bool AlgoSetAlignRegion(int index, const char* jsonRegion, int nSiteColumnId, int nSiteRowId)
{
	if((index<0) || (index>=MAX_THREAD_NUM))
		return false;
	algoInstance[index].SetAlignRegion(jsonRegion, nSiteColumnId, nSiteRowId);
	return true;
}
bool AlgoSetAlignMinScore(int index, float alignScore)
{
	if((index<0) || (index>=MAX_THREAD_NUM))
		return false;
	
	algoInstance[index].SetAlignMinScore(alignScore);

	return true;
}
bool AlgoSetSiteRect(int index, Rect_t siteRect, Param_t param, ThreshGroup_t thresh, int nSiteColumnId, int nSiteRowId)
{
	if((index<0) || (index>=MAX_THREAD_NUM))
		return false;
	
	bool ret = algoInstance[index].SetSiteRect(siteRect, param, thresh, nSiteColumnId, nSiteRowId);

	return ret;
}
bool AlgoSetRoI(int index, const char* jsonRoiSetting, int nSiteColumnId, int nSiteRowId)
{
	if((index<0) || (index>=MAX_THREAD_NUM))
		return false;
	
	bool ret = algoInstance[index].SetRoI(jsonRoiSetting, nSiteColumnId, nSiteRowId);

	return ret;
}
bool AlgoSetMatchMode(int index, MatchType mode)
{
	if((index<0) || (index>=MAX_THREAD_NUM))
		return false;
	
	algoInstance[index].SetMatchMode(mode);

	return true;
}

bool AlgoSetRingWidth(int index, int nThres, int nWidth)
{
	if ((index < 0) || (index >= MAX_THREAD_NUM))
		return false;

	algoInstance[index].SetRingWidth(nThres, nWidth);

	return true;
}


bool AlgoSetSaveDefectImage(int index, bool bSaveImage, bool bDrawDefects)
{
	if((index<0) || (index>=MAX_THREAD_NUM))
		return false;
	
	algoInstance[index].SetSaveDefectImage(bSaveImage, bDrawDefects);

	return true;
}
bool AlgoCreateGoldenImage(const char* szGoldenImgPath, unsigned char* pbyGoldenImg, unsigned char* pbyBaseImg, int alignOffX, int alignOffY, const char* jsonRegion)
{
	bool ret = AlgoGoldenAnalysis::CreateGoldenImage(szGoldenImgPath, pbyGoldenImg, pbyBaseImg, alignOffX, alignOffY, jsonRegion);
	
	return ret;
}
bool AlgoSetGoldenImage(int index, unsigned char* pbyGoldenImg, int nSiteColumnId, int nSiteRowId)
{
	if(NULL==pbyGoldenImg)
		return false;

	if((index<0) || (index>=MAX_THREAD_NUM))
		return false;

	bool ret = algoInstance[index].SetGoldenSiteImage(pbyGoldenImg, nSiteColumnId, nSiteRowId);

	return ret;
}
int AlgoDoAnalysis(int index, unsigned char* imageData, int nDieId, int nSiteColumnId, int nSiteRowId)
{
	if((index<0) || (index>=MAX_THREAD_NUM))
		return INPUT_ERROR;
	
	AlgorithmErrorType err = algoInstance[index].Analysis(imageData, nDieId, nSiteColumnId, nSiteRowId);
	
	return (int)err;
}
AlgorithmErrorType AlgoDoAnalysisInner(int index, unsigned char* imageData, int nDieId, int nSiteColumnId, int nSiteRowId, std::vector<defectDescription<float> >& outputResult, unsigned char* resultData)
{
	if((index<0) || (index>=MAX_THREAD_NUM))
		return INPUT_ERROR;
	
	AlgorithmErrorType err = algoInstance[index].AnalysisInner(imageData, nDieId, nSiteColumnId, nSiteRowId, outputResult, resultData);
	
	return err;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool AlgoSetClustering(int index, bool bEnableClustering, unsigned int nRadius)
{
	if((index<0) || (index>=MAX_THREAD_NUM))
		return false;
	
	algoInstance[index].SetClustering(bEnableClustering, nRadius);

	return true;
}

