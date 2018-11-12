#pragma once

#include "ComDef.h"
#include <vector>
#include <string>
#include "DefectsAnalysis.h"

// ------------------------------------------------------------
// Description : 将stl容器数据转换为json格式的数据
// Parameters :  
// Return Value :
// ------------------------------------------------------------
bool DefectDescriptionStructToJson(std::vector<defectDescription<float> >& resultData,
	std::string& jsonResult);

// ------------------------------------------------------------
// Description : 将stl容器数据转换为json格式的数据文件
// Parameters :  resultData: 检出缺陷数据
//				 jsonResult: 【输出】json格式的数据文件的路径
// Return Value :
// ------------------------------------------------------------
bool DefectDescriptionStructToJsonFile(std::vector<defectDescription<float> >& resultData,
	std::string& jsonFilePath);

void RoiSettingJsonToStruct(const char* jsonROI, ROI_t& roiSetting);

