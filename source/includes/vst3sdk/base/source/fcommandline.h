//------------------------------------------------------------------------
// Project     : SDK Base
// Version     : 1.0
//
// Category    : Helpers
// Filename    : base/source/fcommandline.h
// Created by  : Steinberg, 2007
// Description : Very simple command-line parser.
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

//------------------------------------------------------------------------
/** @file base/source/fcommandline.h
	Very simple command-line parser.
	@see Steinberg::CommandLine */
//------------------------------------------------------------------------
#pragma once

#include <deque>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

namespace Steinberg {
//------------------------------------------------------------------------
/** Very simple command-line parser.

Parses the command-line into a CommandLine::VariablesMap.\n
The command-line parser uses CommandLine::Descriptions to define the available options.

@b Example: 
\code
#include "base/source/fcommandline.h"
#include <iostream>

int main (int argc, char* argv[])
{
	using namespace std;
	using namespace Steinberg;

	CommandLine::Descriptions desc;
	CommandLine::VariablesMap valueMap;

	desc.addOptions ("myTool")
		("help", "produce help message")
		("opt1", string(), "option 1")
		("opt2", string(), "option 2")
	;
	CommandLine::parse (argc, argv, desc, valueMap);

	if (valueMap.hasError () || valueMap.count ("help"))
	{
		cout << desc << "\n";
		return 1;
	}			
	if (valueMap.count ("opt1"))
	{
		cout << "Value of option 1 " << valueMap["opt1"] << "\n";
	}
	if (valueMap.count ("opt2"))
	{
		cout << "Value of option 2 " << valueMap["opt2"] << "\n";
	}
	return 0;
}
\endcode
@note
This is a "header only" implementation.\n
If you need the declarations in more than one cpp file, you have to define 
@c SMTG_NO_IMPLEMENTATION in all but one file.

*/
//------------------------------------------------------------------------
namespace CommandLine {
	
	//------------------------------------------------------------------------
	/**	Command-line parsing result. 
		
		This is the result of the parser.\n
		- Use hasError() to check for errors.\n
		- To test if a option was specified on the command-line use: count()\n
		- To retrieve the value of an options, use operator [](const VariablesMapContainer::key_type k)\n
	*/
	//------------------------------------------------------------------------
	class VariablesMap
	{
		bool mParaError;
		typedef std::map<std::string, std::string> VariablesMapContainer;
		VariablesMapContainer mVariablesMapContainer;
	public:
		VariablesMap () : mParaError (false) {}								///< Constructor. Creates a empty VariablesMap.
		bool hasError () const { return mParaError; }						///< Returns @c true when an error has occurred. 
		void setError () { mParaError = true; }								///< Sets the error state to @c true.
		std::string& operator [](const VariablesMapContainer::key_type k);	///< Retrieve the value of option @c k.
		const std::string& operator [](const VariablesMapContainer::key_type k) const;	///< Retrieve the value of option @c k.
		VariablesMapContainer::size_type count (const VariablesMapContainer::key_type k) const; ///< Returns @c != @c 0 if command-line contains option @c k.
	};

	//! type of the list of elements on the command line that are not handled by options parsing
	typedef std::vector<std::string> FilesVector;

	//------------------------------------------------------------------------
	/** The description of one single command-line option. 
		
		Normally you rarely use a Description directly.\n
		In most cases you will use the Descriptions::addOptions (const std::string&) method to create and add descriptions.
	*/
	//------------------------------------------------------------------------
	class Description : public std::string
	{
	public:
		Description (const std::string& name, const std::string& help, const std::string& valueType ); ///< Construct a Description 
		std::string mHelp; ///< The help string for this option.
		std::string mType; ///< The type of this option (kBool, kString).
		static const std::string kBool;
		static const std::string kString;
	};
	//------------------------------------------------------------------------
	/** List of command-line option descriptions. 
	
		Use addOptions(const std::string&) to add Descriptions.
	*/
	//------------------------------------------------------------------------
	class Descriptions 
	{
		typedef std::deque<Description> DescriptionsList;
		DescriptionsList mDescriptions;
		std::string mCaption;
	public:
		Descriptions& addOptions (const std::string& caption = "");  ///< Sets the command-line tool caption and starts adding Descriptions.
		bool parse (int ac, char* av[], VariablesMap& result, FilesVector* files = 0) const; ///< Parse the command-line.
		void print (std::ostream& os) const;						 ///< Print a brief description for the command-line tool into the stream @c os.
		Descriptions& operator() (const std::string& name, const std::string& help); ///< Add a new switch. Only 
		template <typename Type> Descriptions& operator() (const std::string& name, const Type& inType, std::string help);  ///< Add a new option of type @c inType. Currently only std::string is supported.
	};
	
//------------------------------------------------------------------------
// If you need the declarations in more than one cpp file you have to define 
// SMTG_NO_IMPLEMENTATION in all but one file.
//------------------------------------------------------------------------
#ifndef SMTG_NO_IMPLEMENTATION

	//------------------------------------------------------------------------
	/*! If command-line contains option @c k more than once, only the last value will survive. */
	//------------------------------------------------------------------------
	std::string& VariablesMap::operator [](const VariablesMapContainer::key_type k) 
	{ 
		return mVariablesMapContainer[k]; 
	} 

	//------------------------------------------------------------------------
	/*! If command-line contains option @c k more than once, only the last value will survive. */
	//------------------------------------------------------------------------
	const std::string& VariablesMap::operator [](const VariablesMapContainer::key_type k) const
	{ 
		return (*const_cast<VariablesMap*>(this))[k]; 
	} 
	
	//------------------------------------------------------------------------
	VariablesMap::VariablesMapContainer::size_type VariablesMap::count (const VariablesMapContainer::key_type k) const
	{ 
		return mVariablesMapContainer.count (k); 
	} 

	//------------------------------------------------------------------------
	/** Add a new option with a string as parameter. */
	//------------------------------------------------------------------------
	template <> Descriptions& Descriptions::operator() (const std::string& name, const std::string& inType, std::string help)
	{
		mDescriptions.push_back (Description (name, help, inType));
		return *this;
	}
	bool parse (int ac, char* av[], const Descriptions& desc, VariablesMap& result, FilesVector* files = 0); ///< Parse the command-line.
    std::ostream& operator<< (std::ostream& os, const Descriptions& desc);			 ///< Make Descriptions stream able.

	const std::string Description::kBool = "bool";
	const std::string Description::kString = "string";

	//------------------------------------------------------------------------
	/*! In most cases you will use the Descriptions::addOptions (const std::string&) method to create and add descriptions.
		
		@param[in] name of the option.
		@param[in] help a help description for this option.
		@param[out] valueType Description::kBool or Description::kString. 
	*/
	//------------------------------------------------------------------------
	Description::Description (const std::string& name, const std::string& help, const std::string& valueType) 
	: std::string (name)
	, mHelp (help) 
	, mType (valueType)
	{
	}

	//------------------------------------------------------------------------
	/*! Returning a reverence to *this, enables chaining of calls to operator()(const std::string&, const std::string&).
		
		@param[in] name of the added option.
		@param[in] help a help description for this option.
		@return a reverence to *this.
	*/
	Descriptions& Descriptions::operator() (const std::string& name, const std::string& help)
	{
		mDescriptions.push_back (Description (name, help, Description::kBool));
		return *this;
	}
	
	//------------------------------------------------------------------------
	/*!	<b>Usage example:</b>
																@code
	CommandLine::Descriptions desc;
	desc.addOptions ("myTool")           // Set caption to "myTool"
	    ("help", "produce help message") // add switch -help
	    ("opt1", string(), "option 1")   // add string option -opt1 
	    ("opt2", string(), "option 2")   // add string option -opt2 
	;
																@endcode
	@note
		The operator() is used for every additional option.
	@param[in] caption the caption of the command-line tool.
	@return a reverense to *this. 
	*/
	//------------------------------------------------------------------------
	Descriptions& Descriptions::addOptions (const std::string& caption)
	{
		mCaption = caption;
		return *this;
	}

	//------------------------------------------------------------------------
	/*!	@param[in] ac count of command-line parameters
		@param[in] av command-line as array of strings 
		@param[out] result the parsing result
		@param[out] files optional list of elements on the command line that are not handled by options parsing
	*/
	//------------------------------------------------------------------------
	bool Descriptions::parse (int ac, char* av[], VariablesMap& result, FilesVector* files) const
	{
		using namespace std;

		int i;
		for (i = 1; i < ac; i++)
		{
			string current = av[i];
			if (current[0] == '-')
			{
				int pos = current[1] == '-' ? 2 : 1;
				current = current.substr (pos, string::npos);

				DescriptionsList::const_iterator found = 
					find (mDescriptions.begin (), mDescriptions.end (), current);
				if (found != mDescriptions.end ())
				{
					result[*found] = "true";
					if (found->mType != Description::kBool)
					{
						if (((i + 1) < ac) && *av[i + 1] != '-')
						{	
							result[*found] = av[++i];
						}
						else
						{
							result[*found] = "error!";
							result.setError ();
							return false;
						}
					}
				}
				else
				{
					result.setError ();
					return false;
				}

			}
			else if (files)
				files->push_back (av[i]);
		}
		return true;
	}
	//------------------------------------------------------------------------
	/*!	The description includes the help strings for all options. */
	//------------------------------------------------------------------------
	void Descriptions::print (std::ostream& os) const
	{
		if (!mCaption.empty())
			os << mCaption << ":\n";

		unsigned int i; 
		for (i = 0; i < mDescriptions.size (); ++i)
		{
			const Description& opt = mDescriptions[i];
			os << "-" << opt << ":\t" << opt.mHelp << "\n";
		}
	}
	
	//------------------------------------------------------------------------
    std::ostream& operator<< (std::ostream& os, const Descriptions& desc)
	{
        desc.print (os);
        return os;
	}

	//------------------------------------------------------------------------
	/*! @param[in] ac count of command-line parameters
		@param[in] av command-line as array of strings 
		@param[in] desc Descriptions including all allowed options
		@param[out] result the parsing result		
		@param[out] files optional list of elements on the command line that are not handled by options parsing
	*/
	bool parse (int ac, char* av[], const Descriptions& desc, VariablesMap& result, FilesVector* files)
	{
		return desc.parse (ac, av, result, files);
	}
#endif

} //namespace CommandLine
} //namespace Steinberg
