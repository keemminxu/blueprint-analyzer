// Copyright (c) 2025 keemminxu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/UIAction.h"

class FBlueprintAnalyzerMenuExtension
{
public:
    static void Initialize();
    static void Shutdown();

private:
    static void RegisterMenuExtensions();
    
    // Original Blueprint Analysis Functions
    static void ExecuteAnalyzeBlueprint();
    static void ExecuteExportToJSON();
    static void ExecuteExportToLLMText();
    
    // New Widget Blueprint Analysis Functions
    static void ExecuteAnalyzeWidgetBlueprint();
    static void ExecuteExportWidgetToJSON();
    static void ExecuteExportWidgetToLLMText();
    //static void ExecuteGenerateOptimizedCode();

    // Phase 3: Blueprint Performance Analysis
    static void ExecuteAnalyzeBlueprintPerformance();
    static void ExecuteExportPerformanceToJSON();
    static void ExecuteExportPerformanceToLLMText();

    // Phase 4: Project Folder Analysis
    static void ExecuteAnalyzeFolder();
    static void ExecuteExportProjectToJSON();
    static void ExecuteExportProjectToLLMText();
    static FString GetSelectedFolderPath();
    
    static class UBlueprint* GetSelectedBlueprint();
    static bool IsWidgetBlueprint(UBlueprint* Blueprint);
    
    static FString ShowSaveFileDialog(const FString& DefaultFilename, const FString& FileTypes);
};