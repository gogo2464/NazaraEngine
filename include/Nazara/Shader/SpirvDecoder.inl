// Copyright (C) 2022 Jérôme "Lynix" Leclercq (lynix680@gmail.com)
// This file is part of the "Nazara Engine - Shader module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Shader/SpirvDecoder.hpp>
#include <Nazara/Shader/Debug.hpp>

namespace Nz
{
	inline const UInt32* SpirvDecoder::GetCurrentPtr() const
	{
		return m_currentCodepoint;
	}
}

#include <Nazara/Shader/DebugOff.hpp>
