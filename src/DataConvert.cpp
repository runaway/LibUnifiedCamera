#include "stdafx.h"
#include "DataConvert.h"
#include "writer.h"
#include "document.h"
#include "stringbuffer.h"
#include <fstream>


using namespace rapidjson;
using namespace std;

// ------------------------------------------------------------
// Description : 将stl容器数据转换为json格式的数据
// Parameters :  resultData: 检出缺陷数据
//				 jsonResult: 【输出】json格式的数据
// Return Value :
// ------------------------------------------------------------
bool DefectDescriptionStructToJson(std::vector<defectDescription<float> >& resultData,
	std::string& jsonResult)
{
	if (resultData.empty())
	{
		return false;
	}

	Document doc;
	doc.SetObject();
	Document::AllocatorType& allocator = doc.GetAllocator();

	Value infoArray(kArrayType);
	std::vector<defectDescription<float> >::iterator it = resultData.begin();

    for (; it != resultData.end(); ++it)
	{
		Value infoObject(kArrayType);
		infoObject.SetObject();
		
		Value valStrategyName(kStringType);
		valStrategyName.SetString(it->strategyName.c_str(),
			it->strategyName.size(), allocator);
		infoObject.AddMember("BinningStrategy", valStrategyName,
			allocator);

		Value valBinName(kStringType);
		valBinName.SetString(it->binName.c_str(),
			it->binName.size(), allocator);
		infoObject.AddMember("BinName", valBinName, allocator);

		Value valClassName(kStringType);
		valClassName.SetString(it->className.c_str(),
			it->className.size(), allocator);
		infoObject.AddMember("ClassName", valClassName, allocator);
		
		infoObject.AddMember("ClusterID", it->clusterID, allocator);
		infoObject.AddMember("CenterX", it->defectData.centerX, allocator);
		infoObject.AddMember("CenterY", it->defectData.centerY, allocator);
		infoObject.AddMember("Area", it->defectData.area, allocator);
		infoObject.AddMember("Length", it->defectData.length, allocator);
		infoObject.AddMember("Width", it->defectData.width, allocator);
		infoObject.AddMember("Angle", it->defectData.angle, allocator);
		infoObject.AddMember("AspectRatio", it->defectData.aspectRatio, allocator);
		infoObject.AddMember("Brightness", it->defectData.brightness, allocator);

		infoArray.PushBack(infoObject, allocator);
	}
    
	doc.AddMember("data", infoArray, allocator);
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	doc.Accept(writer);
	
	jsonResult = buffer.GetString();
	
	return true;
}

// ------------------------------------------------------------
// Description : 将stl容器数据转换为json格式的数据文件
// Parameters :  resultData: 检出缺陷数据
//				 jsonResult: 【输出】json格式的数据文件的路径
// Return Value :
// ------------------------------------------------------------
bool DefectDescriptionStructToJsonFile(std::vector<defectDescription<float> >& resultData,
	std::string& savePath)
{
	string jsonData;

	if (!DefectDescriptionStructToJson(resultData, jsonData))
	{
		return false;
	}
	
	ofstream ofs(savePath.c_str());

	ofs << jsonData;
	ofs.close();

	return true;
}

void RoiSettingJsonToStruct(const char* jsonROI, ROI_t& roiSetting)
{
	Document doc;
	doc.Parse(jsonROI);

	roiSetting.polyNum = doc["polyNum"].GetInt();
	roiSetting.circleNum = doc["circleNum"].GetInt();

	if(roiSetting.polyNum > 0)
	{
		roiSetting.polyArray = new Polygon_t [roiSetting.polyNum];	//注意: 释放内存，防止内存泄漏

		Value& polyArray = doc["polyArray"];

		for(int i=0; i<roiSetting.polyNum; i++)
		{	
			bool isInclude = polyArray[i]["IsInclude"].GetBool();
			roiSetting.polyArray[i].IsInclude = isInclude;
			
			int vertexNum = polyArray[i]["vertexNum"].GetInt();
			
		    roiSetting.polyArray[i].vertexNum = vertexNum;			
			roiSetting.polyArray[i].vertexArray =  new Point_t [vertexNum];		//注意: 释放内存，防止内存泄漏
			
			Value& vertexArray = polyArray[i]["vertexArray"];
			for(int j=0; j<vertexNum; j++)
			{
				roiSetting.polyArray[i].vertexArray[j].x = vertexArray[j]["x"].GetFloat();
				roiSetting.polyArray[i].vertexArray[j].y = vertexArray[j]["y"].GetFloat();
			}

			Value& param = polyArray[i]["param"];

			//roiSetting.polyArray[i].param.manualThreshold = param["manualThreshold"].GetFloat();
			roiSetting.polyArray[i].param.minSize = param["minSize"].GetFloat();
			roiSetting.polyArray[i].param.minArea = param["minArea"].GetFloat();
			roiSetting.polyArray[i].param.minWidth = param["minWidth"].GetFloat();
			roiSetting.polyArray[i].param.minLength = param["minLength"].GetFloat();
			roiSetting.polyArray[i].param.maxWidth = param["maxWidth"].GetFloat();
			roiSetting.polyArray[i].param.maxLength = param["maxLength"].GetFloat();
		}
	}
	else
	{
		roiSetting.polyArray = NULL;
	}

	if(roiSetting.circleNum > 0)
	{
		roiSetting.circleArray = new Circle_t [roiSetting.circleNum];	//注意: 释放内存，防止内存泄漏

		Value& circleArray = doc["circleArray"];

		for(int i=0; i<roiSetting.circleNum; i++)
		{
			bool isInclude = circleArray[i]["IsInclude"].GetBool();
			roiSetting.circleArray[i].IsInclude = isInclude;
			
			Value& center = circleArray[i]["center"];
			roiSetting.circleArray[i].center.x = center["x"].GetFloat();
			roiSetting.circleArray[i].center.y = center["y"].GetFloat();

            roiSetting.circleArray[i].radius = circleArray[i]["radius"].GetFloat();

			Value& param = circleArray[i]["param"];

			//roiSetting.circleArray[i].param.manualThreshold = param["manualThreshold"].GetFloat();
			roiSetting.circleArray[i].param.minSize = param["minSize"].GetFloat();
			roiSetting.circleArray[i].param.minArea = param["minArea"].GetFloat();
			roiSetting.circleArray[i].param.minWidth = param["minWidth"].GetFloat();
			roiSetting.circleArray[i].param.minLength = param["minLength"].GetFloat();
			roiSetting.circleArray[i].param.maxWidth = param["maxWidth"].GetFloat();
			roiSetting.circleArray[i].param.maxLength = param["maxLength"].GetFloat();
		}
	}
	else
	{
		roiSetting.circleArray = NULL;
	}
}