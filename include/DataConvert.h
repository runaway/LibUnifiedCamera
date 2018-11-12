#pragma once

#include "ComDef.h"
#include <vector>
#include <string>
#include "DefectsAnalysis.h"

// ------------------------------------------------------------
// Description : ��stl��������ת��Ϊjson��ʽ������
// Parameters :  
// Return Value :
// ------------------------------------------------------------
bool DefectDescriptionStructToJson(std::vector<defectDescription<float> >& resultData,
	std::string& jsonResult);

// ------------------------------------------------------------
// Description : ��stl��������ת��Ϊjson��ʽ�������ļ�
// Parameters :  resultData: ���ȱ������
//				 jsonResult: �������json��ʽ�������ļ���·��
// Return Value :
// ------------------------------------------------------------
bool DefectDescriptionStructToJsonFile(std::vector<defectDescription<float> >& resultData,
	std::string& jsonFilePath);

void RoiSettingJsonToStruct(const char* jsonROI, ROI_t& roiSetting);

