// Copyright (c) 2025 keemminxu. All Rights Reserved.

#include "BlueprintAnalyzer.h"
#include "BlueprintAnalyzerMenuExtension.h"

#define LOCTEXT_NAMESPACE "FBlueprintAnalyzerModule"

void FBlueprintAnalyzerModule::StartupModule()
{
	FBlueprintAnalyzerMenuExtension::Initialize();
}

void FBlueprintAnalyzerModule::ShutdownModule()
{
	FBlueprintAnalyzerMenuExtension::Shutdown();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FBlueprintAnalyzerModule, BlueprintAnalyzer)