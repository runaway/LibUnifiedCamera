#pragma once

#include "ComDef.h"
#include "BinParam.h"
#include <list>
#include <string>

// 用户定义的以实际物理尺寸为单位的Strategy
typedef std::list< BinParam<float> > StrategyList;


class DefectBinningStrategy
{
public:
	DefectBinningStrategy();
	~DefectBinningStrategy();

	
	// ------------------------------------------------------------
	// Description : 加载策略配置文件
	// Parameters :  strategyName：strategy名称
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool LoadStrategyFile(const std::string& strategyName);

	// ------------------------------------------------------------
	// Description : 获取策略配置列表
	// Parameters :  无
	// Return Value :策略配置列表
	// ------------------------------------------------------------
	StrategyList& GetStrategyList(void);

	// ------------------------------------------------------------
	// Description : 获取策略配置列表
	// Parameters :  strategyName: Strategy 名称
	//				 outputList: 【输出值】策略参数表
	// Return Value :策略配置列表
	// ------------------------------------------------------------
	bool GetStrategyList(const std::string& strategyName,
		StrategyList& outputList);

	// ------------------------------------------------------------
	// Description : 保存当前策略配置到文件
	// Parameters :  path: 文件路径
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool SaveStrategyToFile(char* path);

	// ------------------------------------------------------------
	// Description : 将一个bin加入strategy
	// Parameters :  binParam: bin参数
	//				 nOrder: 加入的顺序。默认值-1表示加入到当前
	// strategy的最后
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool AddBinToStrategy(const BinParam<float>& binParam, int nOrder = -1);
	
	// ------------------------------------------------------------
	// Description : 从strategy删除一个bin
	// Parameters :  nID: 待删除bin在strategy中的位置
	// Return Value :true - 成功
	//				 false - 失败
	// ------------------------------------------------------------
	bool DeleteBinFromStrategy(unsigned int nID);

	// ------------------------------------------------------------
	// Description : 按照名称从strategy删除bin
	// Parameters :  binName: 待删除的bin的名称
	// Return Value :
	// ------------------------------------------------------------
	bool DeleteBinFromStrategy(const std::string& binName);

private:
	StrategyList m_strategyList;	// 策略配置列表
};


