/***********************************************************************************************************************
*                                                                                                                      *
* libscopehal                                                                                                          *
*                                                                                                                      *
* Copyright (c) 2012-2024 Andrew D. Zonenberg and contributors                                                         *
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
#ifndef AbstractRenderingEnvironment_h
#define AbstractRenderingEnvironment_h

#include "nfd.h"

#include <optional>
#include <string>
#include <vector>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

/**
	@brief Abstract base for objects (usually filters) which provide a series of actions a user can perform.
	@ingroup core

	Actions are identified by ASCII text names, which must be unique within a given object.

	Most objects provide only one or two actions, so a reasonable user interface is one button per action.
 */
class AbstractRenderingEnvironment
{
public:
	virtual ~AbstractRenderingEnvironment() = default;
	virtual bool PerformInitialSetup() = 0;
	virtual std::optional<std::vector<const char*>> GetNecessaryVulkanExtensions() = 0;
	virtual void TerminateRenderingEnvironment() = 0;
	virtual std::optional<unsigned> GetPrimaryMonitorRefreshRate() = 0;

	class NativeImage
	{
	protected:
		unsigned m_width;
		unsigned m_height;
		std::vector<uint8_t> m_pixels;

	public:
		NativeImage(const unsigned width, const unsigned height, const std::vector<uint8_t>& pixels)
			: m_width(width), m_height(height), m_pixels(pixels)
		{
		}
		[[nodiscard]] unsigned GetWidth() const { return m_width; }
		[[nodiscard]] unsigned GetHeight() const { return m_height; }
		[[nodiscard]] const std::vector<uint8_t>& GetRawPixelsLeftToRightTopToBottom() const { return m_pixels; }
	};

	class Window
	{
	public:
		virtual ~Window() = default;
		virtual bool InitForVulkan(bool installCallbacks) = 0;
		virtual bool UpdateWindowTitle(const std::string& newTitle) = 0;
		virtual bool UpdateWindowSize(unsigned width, unsigned height) = 0;
		virtual bool GetFrameBufferSize(unsigned& width, unsigned& height) = 0;
		virtual std::shared_ptr<vk::raii::SurfaceKHR> GetVulkanSurface(
			std::unique_ptr<vk::raii::Instance>& vkInstance) = 0;
		virtual bool RequestClose() = 0;
		virtual bool ShallClose() = 0;
		virtual void Destroy() = 0;
		virtual void ShutdownImGUI() = 0;
		virtual void NewFrame() = 0;
		virtual bool IsMinimized() = 0;
		//TODO make this conditional when the scopehal-ngscopeclient detachment may come, as this is the line which is the reason that nfd is linked to scopehal
		virtual std::optional<nfdwindowhandle_t> GetNFDWindowHandle() = 0;
		virtual void SetIconSet(const std::vector<std::shared_ptr<const NativeImage>>& icons) = 0;
		virtual void SetFullscreen(bool fullscreen) = 0;
		virtual bool IsFullscreen() = 0;
	};

	virtual std::optional<std::shared_ptr<Window>> CreateWindow(
		unsigned width, unsigned height, const std::string& title) = 0;
};

#endif
