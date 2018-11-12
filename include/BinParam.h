#pragma once

#include "ComDef.h"

// 参数上下限
template<typename T>
class BinParamRange
{
public:
	inline BinParamRange() :
		lowerBound(0),
		upperBound(0)
	{
	}

	inline BinParamRange(T lower, T upper) :
		lowerBound(lower),
		upperBound(upper)
	{
	}

	// 是否范围的上下限都不为0
	inline bool IsRangeValid()
	{
		return (lowerBound >= 0 && upperBound > 0);
	}

	inline bool IsInRange(T data)
	{
		return (lowerBound <= data && upperBound >= data);
	}

	T lowerBound;		// 下限
	T upperBound;		// 上限
};

// 缺陷分类描述参数
template<typename T>
class BinParam
{
public:

	inline BinParam(std::string binN,
		BinParamRange<T> area,
		BinParamRange<float> aspectRatio,
		BinParamRange<unsigned char> brightness,
		BinParamRange<T> size,
		std::string classN) :
		areaRange(area),
		aspectRatioRange(aspectRatio),
		brightnessRange(brightness),
		sizeRange(size),
		className(classN),
		binName(binN)
	{
	}

	inline BinParam() :
		areaRange(BinParamRange<T>(0, 0)),
		aspectRatioRange(BinParamRange<float>(0, 0)),
		brightnessRange(BinParamRange<unsigned char>(0, 0)),
		sizeRange(BinParamRange<T>(0, 0)),
		className(""),
		binName("")
	{

	}

	// ------------------------------------------------------------
	// Description : 判断当前缺陷参数是否满足bin的条件
	// Parameters :  bin: bin参数
	//				 defectData: 缺陷参数
	// Return Value :true - 满足
	//				 false - 不满足
	// ------------------------------------------------------------
	inline bool MeetBinCriteria(DefectParam<T>& defectData)
	{
		// 范围上下限为0，则认为该项不检
		char mask = 0x0;	// 在二进制数据中，哪一位不为零表示该项对应的参数需要判断
		char res = 0x0;		// 在二进制数据中，哪一位不为零表示该项对应的参数落在bin中

		if (areaRange.IsRangeValid())
		{
			mask |= 0x1;
			res |= areaRange.IsInRange(defectData.area) ? 0x1 : 0;
		}

		if (aspectRatioRange.IsRangeValid())
		{
			mask |= 0x2;
			res |= aspectRatioRange.IsInRange(defectData.aspectRatio) ? 0x2 : 0;
		}

		if (brightnessRange.IsRangeValid())
		{
			mask |= 0x4;
			res |= brightnessRange.IsInRange(defectData.brightness) ? 0x4 : 0;
		}

		if (sizeRange.IsRangeValid())
		{
			mask |= 0x8;
			res |= sizeRange.IsInRange(defectData.length) ? 0x8 : 0;
		}
		
		return res == mask;
	}

	BinParamRange<T> areaRange;						// 面积
	BinParamRange<float> aspectRatioRange;			// 长宽比
	BinParamRange<unsigned char> brightnessRange;	// 亮度
	BinParamRange<T> sizeRange;						// 尺寸
	std::string className;							// 分类的名称
	std::string binName;							// 当前bin的名称
};
