//-----------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/hosting/module.cpp
// Created by  : Steinberg, 08/2016
// Description : hosting module classes
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2017, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "module.h"
#include "stringconvert.h"
#include <sstream>
#include <utility>

//------------------------------------------------------------------------
namespace VST3 {
namespace Hosting {

//------------------------------------------------------------------------
FactoryInfo::FactoryInfo (PFactoryInfo&& other) noexcept
{
	*this = std::move (other);
}

//------------------------------------------------------------------------
FactoryInfo& FactoryInfo::operator= (FactoryInfo&& other) noexcept
{
	info = std::move (other.info);
	other.info = {};
	return *this;
}

//------------------------------------------------------------------------
FactoryInfo& FactoryInfo::operator= (PFactoryInfo&& other) noexcept
{
	info = std::move (other);
	other = {};
	return *this;
}

//------------------------------------------------------------------------
std::string FactoryInfo::vendor () const noexcept
{
	return StringConvert::convert (info.vendor, PFactoryInfo::kNameSize);
}

//------------------------------------------------------------------------
std::string FactoryInfo::url () const noexcept
{
	return StringConvert::convert (info.url, PFactoryInfo::kURLSize);
}

//------------------------------------------------------------------------
std::string FactoryInfo::email () const noexcept
{
	return StringConvert::convert (info.email, PFactoryInfo::kEmailSize);
}

//------------------------------------------------------------------------
Steinberg::int32 FactoryInfo::flags () const noexcept
{
	return info.flags;
}

//------------------------------------------------------------------------
bool FactoryInfo::classesDiscardable () const noexcept
{
	return (info.flags & PFactoryInfo::kClassesDiscardable) != 0;
}

//------------------------------------------------------------------------
bool FactoryInfo::licenseCheck () const noexcept
{
	return (info.flags & PFactoryInfo::kLicenseCheck) != 0;
}

//------------------------------------------------------------------------
bool FactoryInfo::componentNonDiscardable () const noexcept
{
	return (info.flags & PFactoryInfo::kComponentNonDiscardable) != 0;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
PluginFactory::PluginFactory (const PluginFactoryPtr& factory) noexcept : factory (factory)
{
}

//------------------------------------------------------------------------
void PluginFactory::setHostContext (Steinberg::FUnknown* context) const noexcept
{
	if (auto f = Steinberg::FUnknownPtr<Steinberg::IPluginFactory3> (factory))
		f->setHostContext (context);
}

//------------------------------------------------------------------------
FactoryInfo PluginFactory::info () const noexcept
{
	Steinberg::PFactoryInfo i;
	factory->getFactoryInfo (&i);
	return FactoryInfo (std::move (i));
}

//------------------------------------------------------------------------
uint32_t PluginFactory::classCount () const noexcept
{
	auto count = factory->countClasses ();
	assert (count >= 0);
	return static_cast<uint32_t> (count);
}

//------------------------------------------------------------------------
PluginFactory::ClassInfos PluginFactory::classInfos () const noexcept
{
	auto count = classCount ();
	ClassInfos classes;
	classes.reserve (count);
	auto f3 = Steinberg::FUnknownPtr<Steinberg::IPluginFactory3> (factory);
	auto f2 = Steinberg::FUnknownPtr<Steinberg::IPluginFactory2> (factory);
	Steinberg::PClassInfo ci;
	Steinberg::PClassInfo2 ci2;
	Steinberg::PClassInfoW ci3;
	for (uint32_t i = 0; i < count; ++i)
	{
		if (f3 && f3->getClassInfoUnicode (i, &ci3) == Steinberg::kResultTrue)
			classes.emplace_back (ci3);
		else if (f2 && f2->getClassInfo2 (i, &ci2) == Steinberg::kResultTrue)
			classes.emplace_back (ci2);
		else if (factory->getClassInfo (i, &ci) == Steinberg::kResultTrue)
			classes.emplace_back (ci);
		auto& classInfo = classes.back ();
		if (classInfo.vendor ().empty ())
			classInfo.get ().vendor = info ().vendor ();
	}
	return classes;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
//------------------------------------------------------------------------
const UID& ClassInfo::ID () const noexcept
{
	return data.classID;
}

//------------------------------------------------------------------------
int32_t ClassInfo::cardinality () const noexcept
{
	return data.cardinality;
}

//------------------------------------------------------------------------
const std::string& ClassInfo::category () const noexcept
{
	return data.category;
}

//------------------------------------------------------------------------
const std::string& ClassInfo::name () const noexcept
{
	return data.name;
}

//------------------------------------------------------------------------
const std::string& ClassInfo::vendor () const noexcept
{
	return data.vendor;
}

//------------------------------------------------------------------------
const std::string& ClassInfo::version () const noexcept
{
	return data.version;
}

//------------------------------------------------------------------------
const std::string& ClassInfo::sdkVersion () const noexcept
{
	return data.sdkVersion;
}

//------------------------------------------------------------------------
const ClassInfo::SubCategories& ClassInfo::subCategories () const noexcept
{
	return data.subCategories;
}

//------------------------------------------------------------------------
Steinberg::uint32 ClassInfo::classFlags () const noexcept
{
	return data.classFlags;
}

//------------------------------------------------------------------------
ClassInfo::ClassInfo (const PClassInfo& info) noexcept
{
	data.classID = info.cid;
	data.cardinality = info.cardinality;
	data.category = StringConvert::convert (info.category, PClassInfo::kCategorySize);
	data.name = StringConvert::convert (info.name, PClassInfo::kNameSize);
}

//------------------------------------------------------------------------
ClassInfo::ClassInfo (const PClassInfo2& info) noexcept
{
	data.classID = info.cid;
	data.cardinality = info.cardinality;
	data.category = StringConvert::convert (info.category, PClassInfo::kCategorySize);
	data.name = StringConvert::convert (info.name, PClassInfo::kNameSize);
	data.vendor = StringConvert::convert (info.vendor, PClassInfo2::kVendorSize);
	data.version = StringConvert::convert (info.version, PClassInfo2::kVersionSize);
	data.sdkVersion = StringConvert::convert (info.sdkVersion, PClassInfo2::kVersionSize);
	parseSubCategories (
	    StringConvert::convert (info.subCategories, PClassInfo2::kSubCategoriesSize));
	data.classFlags = info.classFlags;
}

//------------------------------------------------------------------------
ClassInfo::ClassInfo (const PClassInfoW& info) noexcept
{
	data.classID = info.cid;
	data.cardinality = info.cardinality;
	data.category = StringConvert::convert (info.category, PClassInfo::kCategorySize);
	data.name = StringConvert::convert (info.name, PClassInfo::kNameSize);
	data.vendor = StringConvert::convert (info.vendor, PClassInfo2::kVendorSize);
	data.version = StringConvert::convert (info.version, PClassInfo2::kVersionSize);
	data.sdkVersion = StringConvert::convert (info.sdkVersion, PClassInfo2::kVersionSize);
	parseSubCategories (
	    StringConvert::convert (info.subCategories, PClassInfo2::kSubCategoriesSize));
	data.classFlags = info.classFlags;
}

//------------------------------------------------------------------------
void ClassInfo::parseSubCategories (const std::string& str) noexcept
{
	std::stringstream stream (str);
	std::string item;
	while (std::getline (stream, item, '|'))
		data.subCategories.emplace_back (move (item));
}

//------------------------------------------------------------------------
std::string ClassInfo::subCategoriesString () const noexcept
{
	std::string result;
	if (data.subCategories.empty ())
		return result;
	result = data.subCategories[0];
	for (auto index = 1u; index < data.subCategories.size (); ++index)
		result += "|" + data.subCategories[index];
	return result;
}

//------------------------------------------------------------------------
} // Hosting
} // VST3
