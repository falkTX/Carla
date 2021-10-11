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

#include "jsusfx.h"
#include "jsusfx_serialize.h"

void JsusFxSerializationData::addSlider(const int index, const float value)
{
	sliders.resize(sliders.size() + 1);
	
	Slider & slider = sliders.back();
	slider.index = index;
	slider.value = value;
}

void JsusFxSerializationData::addVar(const float value)
{
	vars.push_back(value);
}

//

JsusFxSerializer_Basic::JsusFxSerializer_Basic(JsusFxSerializationData & _serializationData)
	: JsusFxSerializer()
	, jsusFx(nullptr)
	, write(false)
	, serializationData(_serializationData)
	, varPosition(0)
{
}

void JsusFxSerializer_Basic::begin(JsusFx & _jsusFx, const bool _write)
{
	jsusFx = &_jsusFx;
	write = _write;
	
	varPosition = 0;
	
	if (write)
		saveSliders(*jsusFx, serializationData);
	else
		restoreSliders(*jsusFx, serializationData);
}

void JsusFxSerializer_Basic::end()
{
}

void JsusFxSerializer_Basic::saveSliders(const JsusFx & jsusFx, JsusFxSerializationData & serializationData)
{
	for (int i = 0; i < jsusFx.kMaxSliders; ++i)
	{
		if (jsusFx.sliders[i].exists && jsusFx.sliders[i].getValue() != jsusFx.sliders[i].def)
		{
			serializationData.addSlider(i, jsusFx.sliders[i].getValue());
		}
	}
}

void JsusFxSerializer_Basic::restoreSliders(JsusFx & jsusFx, const JsusFxSerializationData & serializationData)
{
	for (int i = 0; i < JsusFx::kMaxSliders; ++i)
	{
		if (jsusFx.sliders[i].exists)
			jsusFx.sliders[i].setValue(jsusFx.sliders[i].def);
	}
	
	for (int i = 0; i < serializationData.sliders.size(); ++i)
	{
		const JsusFxSerializationData::Slider & slider = serializationData.sliders[i];
		
		if (slider.index >= 0 &&
			slider.index < JsusFx::kMaxSliders &&
			jsusFx.sliders[slider.index].exists)
		{
			jsusFx.sliders[slider.index].setValue(slider.value);
		}
	}
}

int JsusFxSerializer_Basic::file_avail() const
{
	if (write)
		return -1;
	else
		return varPosition == serializationData.vars.size() ? 0 : 1;
}

int JsusFxSerializer_Basic::file_var(EEL_F & value)
{
	if (write)
	{
		serializationData.vars.push_back(value);
		
		return 1;
	}
	else
	{
		if (varPosition >= 0 && varPosition < serializationData.vars.size())
		{
			value = serializationData.vars[varPosition];
			varPosition++;
			return 1;
		}
		else
		{
			value = 0.f;
			return 0;
		}
	}
}

int JsusFxSerializer_Basic::file_mem(EEL_F * values, const int numValues)
{
	if (write)
	{
		for (int i = 0; i < numValues; ++i)
		{
			serializationData.vars.push_back(values[i]);
		}
		return 1;
	}
	else
	{
		if (numValues < 0)
			return 0;
		if (varPosition >= 0 && varPosition + numValues <= serializationData.vars.size())
		{
			for (int i = 0; i < numValues; ++i)
			{
				values[i] = serializationData.vars[varPosition];
				varPosition++;
			}
			return 1;
		}
		else
		{
			for (int i = 0; i < numValues; ++i)
				values[i] = 0.f;
			return 0;
		}
	}
}
