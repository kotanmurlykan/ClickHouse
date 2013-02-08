#pragma once

#include <city.h>

#include <stats/UniquesHashSet.h>

#include <DB/IO/WriteHelpers.h>
#include <DB/IO/ReadHelpers.h>

#include <DB/DataTypes/DataTypesNumberFixed.h>

#include <DB/AggregateFunctions/IUnaryAggregateFunction.h>


namespace DB
{


template <typename T> struct AggregateFunctionUniqTraits;

template <> struct AggregateFunctionUniqTraits<UInt64>
{
	static UInt64 hash(UInt64 x) { return x; }
};

template <> struct AggregateFunctionUniqTraits<Int64>
{
	static UInt64 hash(Int64 x) { return x; }
};

template <> struct AggregateFunctionUniqTraits<Float64>
{
	static UInt64 hash(Float64 x)
	{
		UInt64 res = 0;
		memcpy(reinterpret_cast<char *>(&res), reinterpret_cast<char *>(&x), sizeof(x));
		return res;
	}
};

template <> struct AggregateFunctionUniqTraits<String>
{
	/// Имейте ввиду, что вычисление приближённое.
	static UInt64 hash(const String & x) { return CityHash64(x.data(), x.size()); }
};


struct AggregateFunctionUniqData
{
	UniquesHashSet set;
};


/// Приближённо вычисляет количество различных значений.
template <typename T>
class AggregateFunctionUniq : public IUnaryAggregateFunction<AggregateFunctionUniqData>
{
public:
	AggregateFunctionUniq() {}

	String getName() const { return "uniq"; }
	String getTypeID() const { return "uniq_" + TypeName<T>::get(); }

	DataTypePtr getReturnType() const
	{
		return new DataTypeUInt64;
	}

	void setArgument(const DataTypePtr & argument)
	{
	}


	void addOne(AggregateDataPtr place, const Field & value) const
	{
		data(place).set.insert(AggregateFunctionUniqTraits<T>::hash(get<const T &>(value)));
	}

	void merge(AggregateDataPtr place, ConstAggregateDataPtr rhs) const
	{
		data(place).set.merge(data(rhs).set);
	}

	void serialize(ConstAggregateDataPtr place, WriteBuffer & buf) const
	{
		data(place).set.write(buf);
	}

	void deserializeMerge(AggregateDataPtr place, ReadBuffer & buf) const
	{
		UniquesHashSet tmp_set;
		tmp_set.read(buf);
		data(place).set.merge(tmp_set);
	}

	Field getResult(ConstAggregateDataPtr place) const
	{
		return data(place).set.size();
	}
};

}
