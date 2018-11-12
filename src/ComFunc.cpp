#include "stdafx.h"
#include "ComFunc.h"
#include <windows.h>


// ------------------------------------------------------------
// Description : 获取指定路径下的所有文件名，并将其放入list中
// Parameters :  path: 路径
//				 fileNameList: 文件名list
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool GetFileNameFromDir(const std::string& path, std::list<std::string>& fileNameList)
{
	fileNameList.clear();

	std::string DirPath = path + "*.xml";
	HANDLE file;
	WIN32_FIND_DATA fileData;

	file = FindFirstFile(DirPath.c_str(), &fileData);

	CString fullFileName(fileData.cFileName);
	CString fileTitle = fullFileName.Left(fullFileName.GetLength() - 4);
	fileNameList.push_back(fileTitle.GetBuffer(0));

	while (FindNextFile(file, &fileData))
	{
		CString fullFileName(fileData.cFileName);
		fileTitle = fullFileName.Left(fullFileName.GetLength() - 4);
		fileNameList.push_back(fileTitle.GetBuffer(0));
	}

	return true;
}

// ------------------------------------------------------------
// Description : 获取指定路径下的所有图像文件名，并将其放入vector中
// Parameters :  path: 路径
//				 fileNameList: 文件名list
// Return Value :true - 成功
//				 false - 失败
// ------------------------------------------------------------
bool GetImgNameFromDir(const std::string& path, std::vector<std::string>& fileNameVec)
{
	fileNameVec.clear();

	std::string DirPath = path + "*.bmp";
	HANDLE file;
	WIN32_FIND_DATA fileData;

	file = FindFirstFile(DirPath.c_str(), &fileData);
	
	std::string fileDataName = fileData.cFileName;

    if (fileDataName != "." && fileDataName != "..")
	{
		fileNameVec.push_back(fileData.cFileName);
	}
	
	while (FindNextFile(file, &fileData))
	{	
		std::string fileDataName = fileData.cFileName;

        if (fileDataName != "." && fileDataName != "..")
		{
			fileNameVec.push_back(fileData.cFileName);
		}		
	}
	FindClose(file);
	return true;
}