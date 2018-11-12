#pragma once

#include <string>
#include <list>
#include <vector>


// ------------------------------------------------------------
// Description : 获取指定路径下的所有文件名，并将其放入list中
// Parameters :  path: 路径
//				 fileNameList: 文件名list
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool GetFileNameFromDir(const std::string& path,
	std::list<std::string>& fileNameList);

// ------------------------------------------------------------
// Description : 获取指定路径下的所有图像文件名，并将其放入vector中
// Parameters :  path: 路径
//				 fileNameList: 文件名list
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool GetImgNameFromDir(const std::string& path, std::vector<std::string>& fileNameVec);
