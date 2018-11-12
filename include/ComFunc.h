#pragma once

#include <string>
#include <list>
#include <vector>


// ------------------------------------------------------------
// Description : ��ȡָ��·���µ������ļ��������������list��
// Parameters :  path: ·��
//				 fileNameList: �ļ���list
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool GetFileNameFromDir(const std::string& path,
	std::list<std::string>& fileNameList);

// ------------------------------------------------------------
// Description : ��ȡָ��·���µ�����ͼ���ļ��������������vector��
// Parameters :  path: ·��
//				 fileNameList: �ļ���list
// Return Value :true - �ɹ�
//				 false - ʧ��
// ------------------------------------------------------------
bool GetImgNameFromDir(const std::string& path, std::vector<std::string>& fileNameVec);
