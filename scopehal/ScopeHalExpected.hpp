/***********************************************************************************************************************
*                                                                                                                      *
* libscopehal                                                                                                          *
*                                                                                                                      *
* Copyright (c) 2012-2026 Andrew D. Zonenberg and contributors                                                         *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

/**
	@file
	@author Hegyi László
	@brief Declaration of ActionProvider
	@ingroup core
 */
#ifndef ScopeHalExpected_h
#define ScopeHalExpected_h

#include <cassert>
#include <memory>
#include <string>

/**
	@brief Abstract base for objects (usually filters) which provide a series of actions a user can perform.
	@ingroup core

	Actions are identified by ASCII text names, which must be unique within a given object.

	Most objects provide only one or two actions, so a reasonable user interface is one button per action.
 */

namespace ScopeHalExpectedHolder
{
template<class Y>
struct holder
{
	Y value;
};

template<>
struct holder<void>
{
	int value;
};
}

template<typename T>
class ScopeHalExpected
{
protected:
	//https://stackoverflow.com/a/21904225
	static constexpr bool IsVoid = std::is_same_v<T, void>;

	bool isNormal;
	struct ScopeHalExpectedHolder::holder<T> normalData;

	std::string error;

public:
	template<typename U = T, std::enable_if_t<!IsVoid>>
	ScopeHalExpected(U val)
	{
		isNormal = true;
		if(!IsVoid)
		{
			this->normalData.value = std::move(val);
		}
	}

	ScopeHalExpected() : normalData(), error() { isNormal = true; }

	explicit operator bool() const { return isNormal; }

	[[nodiscard]] std::string getError() const
	{
		assert(!isNormal);
		return error;
	}
	static ScopeHalExpected Error(std::string err)
	{
		ScopeHalExpected e;
		e.error = err;
		return std::move(e);
	}
	template<typename = std::enable_if_t<!IsVoid>>
	[[nodiscard]] auto& operator*()
	{
		assert(isNormal);
		return normalData.value;
	}
};

#endif
