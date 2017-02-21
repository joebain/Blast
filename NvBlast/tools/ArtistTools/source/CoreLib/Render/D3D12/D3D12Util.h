// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright (c) 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//

#pragma once

#include <d3d12.h>

#include "RenderPlugin.h"

class GPUShaderResource;

namespace D3D12Util
{
	///////////////////////////////////////////////////////////////////
	// render window management
	///////////////////////////////////////////////////////////////////
	D3DHandles& GetDeviceHandles(D3DHandles& deviceHandles);
	bool CreateRenderWindow(HWND hWnd, int nSamples);
	void DestroyRenderWindow();
	bool ResizeRenderWindow(int w, int h);
	void PresentRenderWindow();
	void ClearRenderWindow(float r, float g, float b);

	///////////////////////////////////////////////////////////////////
	// background textures
	bool LoadBackgroundTexture(const char* filePath);
	void RenderBackgroundTexture();
	void ClearBackgroundTexture();
	GPUShaderResource* CreateTextureSRV(const char* texturename);

	///////////////////////////////////////////////////////////////////
	bool GetDeviceInfoString(wchar_t *str);

	///////////////////////////////////////////////////////////////////
	// text helpers
	void TxtHelperBegin();
	void TxtHelperEnd();
	void TxtHelperSetInsertionPos(int x, int y);
	void TxtHelperSetForegroundColor(float r, float g, float b, float a = 1.0f);
	void TxtHelperDrawTextLine(wchar_t* str);


}
