// Copyright (c) 2025 keemminxu. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FBlueprintAnalyzerModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};