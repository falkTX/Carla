/*
 * Copyright 2018 Pascal Gauthier, Marcel Smit
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * *distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "WDL/eel2/ns-eel.h"

#include <vector>

class JsusFx;

struct JsusFxSerializationData
{
	// note : Reaper will always save/restore values using single precision floating point, so we don't use EEL_F here,
	// as it may be double or float depending on compile time options
	
	struct Slider
	{
		int index;
		float value;
	};
	
	std::vector<Slider> sliders;
	std::vector<float> vars;
	
	void addSlider(const int index, const float value);
	void addVar(const float value);
	
	bool operator==(const JsusFxSerializationData & other) const
	{
		if (sliders.size() != other.sliders.size())
			return false;
		if (vars.size() != other.vars.size())
			return false;
		
		for (size_t i = 0; i < sliders.size(); ++i)
			if (sliders[i].index != other.sliders[i].index ||
				sliders[i].value != other.sliders[i].value)
				return false;
		
		for (size_t i = 0; i < vars.size(); ++i)
			if (vars[i] != other.vars[i])
				return false;
		
		return true;
	}
	
	bool operator!=(const JsusFxSerializationData & other) const
	{
		return !(*this == other);
	}
};

struct JsusFxSerializer
{
	virtual void begin(JsusFx & _jsusFx, const bool _write) = 0;
	virtual void end() = 0;
	
	virtual int file_avail() const = 0;
	virtual int file_var(EEL_F & value) = 0;
	virtual int file_mem(EEL_F * values, const int numValues) = 0;
};

struct JsusFxSerializer_Basic : JsusFxSerializer
{
	JsusFx * jsusFx;
	bool write;
	
	JsusFxSerializationData & serializationData;
	
	int varPosition;
	
	JsusFxSerializer_Basic(JsusFxSerializationData & _serializationData);
	
	virtual void begin(JsusFx & _jsusFx, const bool _write) override;
	virtual void end() override;
	
	static void saveSliders(const JsusFx & jsusFx, JsusFxSerializationData & serializationData);
	static void restoreSliders(JsusFx & jsusFx, const JsusFxSerializationData & serializationData);
	
	virtual int file_avail() const override;
	virtual int file_var(EEL_F & value) override;
	virtual int file_mem(EEL_F * values, const int numValues) override;
};
