// Copyright (C) 2020 Jérôme Leclercq
// This file is part of the "Nazara Engine - Renderer module"
// For conditions of distribution and use, see copyright notice in Config.hpp

#include <Nazara/Prerequisites.hpp>

#ifdef NAZARA_PLATFORM_WINDOWS

#define NAZARA_REQUEST_DEDICATED_GPU() \
extern "C" \
{ \
	NAZARA_EXPORT unsigned long NvOptimusEnablement = 1; \
	NAZARA_EXPORT unsigned long AmdPowerXpressRequestHighPerformance = 1; \
}

#else

#define NAZARA_REQUEST_DEDICATED_GPU()

#endif
