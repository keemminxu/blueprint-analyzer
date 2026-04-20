// Copyright (c) 2025 keemminxu. All Rights Reserved.

#include "BlueprintAnalyzerMenuExtension.h"
#include "BlueprintAnalyzerLibrary.h"
#include "Engine/Blueprint.h"
#include "Blueprint/UserWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "Misc/MessageDialog.h"
#include "DesktopPlatformModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Misc/FileHelper.h"

void FBlueprintAnalyzerMenuExtension::Initialize()
{
    RegisterMenuExtensions();
}

void FBlueprintAnalyzerMenuExtension::Shutdown()
{
    // Cleanup
}

void FBlueprintAnalyzerMenuExtension::RegisterMenuExtensions()
{
    UToolMenus* ToolMenus = UToolMenus::Get();
    if (!ToolMenus)
    {
        return;
    }

    // Folder context menu (Phase 4: batch analyze)
    if (UToolMenu* FolderMenu = ToolMenus->ExtendMenu("ContentBrowser.FolderContextMenu"))
    {
        FToolMenuSection& FolderSection = FolderMenu->FindOrAddSection("PathContextBulkOperations");
        FolderSection.AddSubMenu(
            "BlueprintAnalyzerFolder",
            FText::FromString("Blueprint Analyzer"),
            FText::FromString("Analyze all Blueprints in this folder"),
            FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
            {
                FToolMenuSection& ProjectSection = SubMenu->AddSection("ProjectAnalysis", FText::FromString("Project Analysis"));
                ProjectSection.AddMenuEntry(
                    "AnalyzeFolder",
                    FText::FromString("Analyze Folder"),
                    FText::FromString("Analyze all Blueprints in this folder and show summary"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeFolder))
                );
                ProjectSection.AddMenuEntry(
                    "ExportProjectToJSON",
                    FText::FromString("Export Project Analysis to JSON"),
                    FText::FromString("Save folder analysis as JSON"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportProjectToJSON))
                );
                ProjectSection.AddMenuEntry(
                    "ExportProjectToLLMText",
                    FText::FromString("Export Project Analysis to LLM Text"),
                    FText::FromString("Save folder analysis as LLM-friendly text"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportProjectToLLMText))
                );
            })
        );
    }

    UToolMenu* ContentBrowserAssetMenu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu.Blueprint");
    if (ContentBrowserAssetMenu)
    {
        FToolMenuSection& Section = ContentBrowserAssetMenu->FindOrAddSection("GetAssetActions");
        Section.AddSubMenu(
            "BlueprintAnalyzer",
            FText::FromString("Blueprint Analyzer"),
            FText::FromString("Blueprint analysis and optimization tools"),
            FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
            {
                FToolMenuSection& SubSection = SubMenu->AddSection("BlueprintAnalyzerActions", FText::FromString("Analysis Actions"));
                
                // Regular Blueprint Analysis
                SubSection.AddMenuEntry(
                    "AnalyzeBlueprint",
                    FText::FromString("Analyze Blueprint"),
                    FText::FromString("Analyze the selected blueprint structure"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeBlueprint))
                );
                
                SubSection.AddMenuEntry(
                    "ExportToJSON",
                    FText::FromString("Export to JSON"),
                    FText::FromString("Export blueprint analysis to JSON format"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportToJSON))
                );
                
                SubSection.AddMenuEntry(
                    "ExportToLLMText",
                    FText::FromString("Export to LLM Text"),
                    FText::FromString("Export blueprint analysis to LLM-friendly text format"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportToLLMText))
                );

                // Blueprint Performance Analysis (Phase 3)
                FToolMenuSection& PerfSection = SubMenu->AddSection("BlueprintPerformanceActions", FText::FromString("Performance Analysis"));

                PerfSection.AddMenuEntry(
                    "AnalyzeBlueprintPerformance",
                    FText::FromString("Analyze Blueprint Performance"),
                    FText::FromString("Detect performance anti-patterns (Tick-heavy calls, Cast abuse, etc.)"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeBlueprintPerformance))
                );

                PerfSection.AddMenuEntry(
                    "ExportPerformanceToJSON",
                    FText::FromString("Export Performance Report to JSON"),
                    FText::FromString("Save performance analysis as JSON"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportPerformanceToJSON))
                );

                PerfSection.AddMenuEntry(
                    "ExportPerformanceToLLMText",
                    FText::FromString("Export Performance Report to LLM Text"),
                    FText::FromString("Save performance analysis as LLM-friendly text"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportPerformanceToLLMText))
                );

                // Widget Blueprint Optimization
                FToolMenuSection& WidgetSection = SubMenu->AddSection("WidgetAnalyzerActions", FText::FromString("Widget Optimization"));
                
                WidgetSection.AddMenuEntry(
                    "AnalyzeWidgetBlueprint",
                    FText::FromString("Analyze Widget Blueprint"),
                    FText::FromString("Analyze widget blueprint for optimization opportunities"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeWidgetBlueprint))
                );
                
                WidgetSection.AddMenuEntry(
                    "ExportWidgetToJSON",
                    FText::FromString("Export Widget Analysis to JSON"),
                    FText::FromString("Export widget optimization report to JSON format"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportWidgetToJSON))
                );
                
                WidgetSection.AddMenuEntry(
                    "ExportWidgetToLLMText",
                    FText::FromString("Export Widget Analysis to LLM Text"),
                    FText::FromString("Export widget optimization report to LLM-friendly text format"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportWidgetToLLMText))
                );
            })
        );
    }
}

void FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeBlueprint()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No blueprint selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FBlueprintAnalysisResult AnalysisResult = UBlueprintAnalyzerLibrary::AnalyzeBlueprint(SelectedBlueprint);

    const FBPAnalyzerMetadata& Meta = AnalysisResult.Metadata;
    FString Message;
    Message += FString::Printf(TEXT("Blueprint '%s' analyzed successfully!\n\n"), *AnalysisResult.BlueprintName);
    Message += FString::Printf(TEXT("Type: %s\n"), *Meta.BlueprintType);
    Message += FString::Printf(TEXT("Parent: %s\n"), *Meta.ParentClass);
    Message += FString::Printf(TEXT("Interfaces: %d\n"), Meta.ImplementedInterfaces.Num());
    Message += FString::Printf(TEXT("Variables: %d\n"), Meta.Variables.Num());
    Message += FString::Printf(TEXT("Custom Functions: %d\n"), Meta.CustomFunctions.Num());
    Message += FString::Printf(TEXT("Components: %d\n"), Meta.Components.Num());
    Message += FString::Printf(TEXT("Event Dispatchers: %d\n"), Meta.EventDispatchers.Num());
    Message += FString::Printf(TEXT("Macros: %d\n"), Meta.MacroNames.Num());
    Message += FString::Printf(TEXT("Timelines: %d\n"), Meta.TimelineNames.Num());
    Message += FString::Printf(TEXT("Nodes: %d\n"), AnalysisResult.Nodes.Num());
    Message += FString::Printf(TEXT("Connections: %d\n"), AnalysisResult.Connections.Num());
    Message += FString::Printf(TEXT("Execution Paths: %d"), AnalysisResult.ExecutionPaths.Num());

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
}

void FBlueprintAnalyzerMenuExtension::ExecuteExportToJSON()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No blueprint selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FString DefaultFilename = FString::Printf(TEXT("%s_Analysis.json"), *SelectedBlueprint->GetName());
    FString SavePath = ShowSaveFileDialog(DefaultFilename, TEXT("JSON Files (*.json)|*.json"));
    
    if (!SavePath.IsEmpty())
    {
        FBlueprintAnalysisResult AnalysisResult = UBlueprintAnalyzerLibrary::AnalyzeBlueprint(SelectedBlueprint);
        bool bSuccess = UBlueprintAnalyzerLibrary::SaveAnalysisToFile(AnalysisResult, SavePath, TEXT("JSON"));
        
        if (bSuccess)
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Analysis exported to: %s"), *SavePath)));
        }
        else
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Failed to export analysis."), FText::FromString("Blueprint Analyzer"));
        }
    }
}

void FBlueprintAnalyzerMenuExtension::ExecuteExportToLLMText()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No blueprint selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FString DefaultFilename = FString::Printf(TEXT("%s_LLM_Analysis.txt"), *SelectedBlueprint->GetName());
    FString SavePath = ShowSaveFileDialog(DefaultFilename, TEXT("Text Files (*.txt)|*.txt"));
    
    if (!SavePath.IsEmpty())
    {
        FBlueprintAnalysisResult AnalysisResult = UBlueprintAnalyzerLibrary::AnalyzeBlueprint(SelectedBlueprint);
        bool bSuccess = UBlueprintAnalyzerLibrary::SaveAnalysisToFile(AnalysisResult, SavePath, TEXT("TEXT"));
        
        if (bSuccess)
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("LLM-friendly analysis exported to: %s"), *SavePath)));
        }
        else
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Failed to export analysis."), FText::FromString("Blueprint Analyzer"));
        }
    }
}

void FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeWidgetBlueprint()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No blueprint selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    if (!IsWidgetBlueprint(SelectedBlueprint))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Selected blueprint is not a Widget Blueprint."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FWidgetOptimizationReport Report = UBlueprintAnalyzerLibrary::AnalyzeWidgetBlueprint(SelectedBlueprint);
    
    FString SeverityText;
    if (Report.OptimizationScore >= 90)
    {
        SeverityText = TEXT("Excellent - Very Well Optimized");
    }
    else if (Report.OptimizationScore >= 70)
    {
        SeverityText = TEXT("Good - Well Optimized");
    }
    else if (Report.OptimizationScore >= 50)
    {
        SeverityText = TEXT("Average - Some Improvements Needed");
    }
    else if (Report.OptimizationScore >= 30)
    {
        SeverityText = TEXT("Poor - Significant Optimization Required");
    }
    else
    {
        SeverityText = TEXT("Critical - Complete Redesign Required");
    }
    
    FString Message = FString::Printf(TEXT("Widget Blueprint '%s' analyzed successfully!\n\n")
        TEXT("Total Widgets: %d\n")
        TEXT("Max Depth: %d\n")
        TEXT("Total Bindings: %d\n")
        TEXT("Optimization Issues: %d\n")
        TEXT("Optimization Score: %d/100 (%s)\n")
        TEXT("Estimated Memory: %.2f KB"), 
        *Report.WidgetBlueprintName,
        Report.TotalWidgets,
        Report.MaxDepth,
        Report.TotalBindings,
        Report.OptimizationIssues.Num(),
        Report.OptimizationScore,
        *SeverityText,
        Report.EstimatedMemoryUsage);
        
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
}

void FBlueprintAnalyzerMenuExtension::ExecuteExportWidgetToJSON()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No blueprint selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    if (!IsWidgetBlueprint(SelectedBlueprint))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Selected blueprint is not a Widget Blueprint."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FString DefaultFilename = FString::Printf(TEXT("%s_WidgetOptimization.json"), *SelectedBlueprint->GetName());
    FString SavePath = ShowSaveFileDialog(DefaultFilename, TEXT("JSON Files (*.json)|*.json"));
    
    if (!SavePath.IsEmpty())
    {
        FWidgetOptimizationReport Report = UBlueprintAnalyzerLibrary::AnalyzeWidgetBlueprint(SelectedBlueprint);
        bool bSuccess = UBlueprintAnalyzerLibrary::SaveWidgetAnalysisToFile(Report, SavePath, TEXT("JSON"));
        
        if (bSuccess)
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Widget optimization report exported to: %s"), *SavePath)));
        }
        else
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Failed to export widget analysis."), FText::FromString("Blueprint Analyzer"));
        }
    }
}

void FBlueprintAnalyzerMenuExtension::ExecuteExportWidgetToLLMText()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No blueprint selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    if (!IsWidgetBlueprint(SelectedBlueprint))
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Selected blueprint is not a Widget Blueprint."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FString DefaultFilename = FString::Printf(TEXT("%s_WidgetOptimization_LLM.txt"), *SelectedBlueprint->GetName());
    FString SavePath = ShowSaveFileDialog(DefaultFilename, TEXT("Text Files (*.txt)|*.txt"));
    
    if (!SavePath.IsEmpty())
    {
        FWidgetOptimizationReport Report = UBlueprintAnalyzerLibrary::AnalyzeWidgetBlueprint(SelectedBlueprint);
        bool bSuccess = UBlueprintAnalyzerLibrary::SaveWidgetAnalysisToFile(Report, SavePath, TEXT("TEXT"));
        
        if (bSuccess)
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("LLM-friendly widget optimization report exported to: %s"), *SavePath)));
        }
        else
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Failed to export widget analysis."), FText::FromString("Blueprint Analyzer"));
        }
    }
}

//void FBlueprintAnalyzerMenuExtension::ExecuteGenerateOptimizedCode()
//{
//    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
//    if (!SelectedBlueprint)
//    {
//        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No blueprint selected."), FText::FromString("Blueprint Analyzer"));
//        return;
//    }
//
//    if (!IsWidgetBlueprint(SelectedBlueprint))
//    {
//        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Selected blueprint is not a Widget Blueprint."), FText::FromString("Blueprint Analyzer"));
//        return;
//    }
//
//    FString DefaultFilename = FString::Printf(TEXT("Optimized%s.h"), *SelectedBlueprint->GetName());
//    FString SavePath = ShowSaveFileDialog(DefaultFilename, TEXT("Header Files (*.h)|*.h"));
//    
//    if (!SavePath.IsEmpty())
//    {
//        FWidgetOptimizationReport Report = UBlueprintAnalyzerLibrary::AnalyzeWidgetBlueprint(SelectedBlueprint);
//        FString OptimizedCode = UBlueprintAnalyzerLibrary::GenerateOptimizedWidgetCode(Report);
//        
//        bool bSuccess = FFileHelper::SaveStringToFile(OptimizedCode, *SavePath);
//        
//        if (bSuccess)
//        {
//            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Optimized C++ code generated at: %s"), *SavePath)));
//        }
//        else
//        {
//            FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ExportFailed", "Failed to generate optimized code."));
//        }
//    }
//}

void FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeBlueprintPerformance()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No blueprint selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FBPPerformanceReport Report = UBlueprintAnalyzerLibrary::AnalyzeBlueprintPerformance(SelectedBlueprint);

    FString Grade;
    if (Report.PerformanceScore >= 90) Grade = TEXT("Excellent");
    else if (Report.PerformanceScore >= 70) Grade = TEXT("Good");
    else if (Report.PerformanceScore >= 50) Grade = TEXT("Average");
    else if (Report.PerformanceScore >= 30) Grade = TEXT("Poor");
    else Grade = TEXT("Critical");

    FString Message;
    Message += FString::Printf(TEXT("Performance Report for '%s'\n\n"), *Report.BlueprintName);
    Message += FString::Printf(TEXT("Score: %d/100 (%s)\n"), Report.PerformanceScore, *Grade);
    Message += FString::Printf(TEXT("Total Nodes: %d\n"), Report.TotalNodes);
    Message += FString::Printf(TEXT("Events: %d\n"), Report.EventCount);
    Message += FString::Printf(TEXT("Casts: %d\n"), Report.CastCount);
    Message += FString::Printf(TEXT("Tick downstream: %d nodes\n"), Report.TickNodeCount);
    Message += FString::Printf(TEXT("BeginPlay downstream: %d nodes\n"), Report.BeginPlayNodeCount);
    Message += FString::Printf(TEXT("Issues Found: %d"), Report.Issues.Num());

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
}

void FBlueprintAnalyzerMenuExtension::ExecuteExportPerformanceToJSON()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No blueprint selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FString DefaultFilename = FString::Printf(TEXT("%s_Performance.json"), *SelectedBlueprint->GetName());
    FString SavePath = ShowSaveFileDialog(DefaultFilename, TEXT("JSON Files (*.json)|*.json"));
    if (SavePath.IsEmpty()) return;

    FBPPerformanceReport Report = UBlueprintAnalyzerLibrary::AnalyzeBlueprintPerformance(SelectedBlueprint);
    const FString Content = UBlueprintAnalyzerLibrary::ExportPerformanceReportToJSON(Report);
    const bool bSuccess = FFileHelper::SaveStringToFile(Content, *SavePath);

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(bSuccess
        ? FString::Printf(TEXT("Performance report exported to: %s"), *SavePath)
        : TEXT("Failed to export performance report.")));
}

void FBlueprintAnalyzerMenuExtension::ExecuteExportPerformanceToLLMText()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No blueprint selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FString DefaultFilename = FString::Printf(TEXT("%s_Performance_LLM.txt"), *SelectedBlueprint->GetName());
    FString SavePath = ShowSaveFileDialog(DefaultFilename, TEXT("Text Files (*.txt)|*.txt"));
    if (SavePath.IsEmpty()) return;

    FBPPerformanceReport Report = UBlueprintAnalyzerLibrary::AnalyzeBlueprintPerformance(SelectedBlueprint);
    const FString Content = UBlueprintAnalyzerLibrary::ExportPerformanceReportToLLMText(Report);
    const bool bSuccess = FFileHelper::SaveStringToFile(Content, *SavePath);

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(bSuccess
        ? FString::Printf(TEXT("LLM-friendly performance report exported to: %s"), *SavePath)
        : TEXT("Failed to export performance report.")));
}

// ============================================================
// Phase 4: Folder Analysis Execute Functions
// ============================================================

FString FBlueprintAnalyzerMenuExtension::GetSelectedFolderPath()
{
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    TArray<FString> SelectedPaths;
    ContentBrowserModule.Get().GetSelectedPathViewFolders(SelectedPaths);

    if (SelectedPaths.Num() > 0)
    {
        return SelectedPaths[0];
    }

    // Fallback: try GetSelectedFolders
    TArray<FString> SelectedFolders;
    ContentBrowserModule.Get().GetSelectedFolders(SelectedFolders);
    if (SelectedFolders.Num() > 0)
    {
        return SelectedFolders[0];
    }

    return FString();
}

void FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeFolder()
{
    const FString FolderPath = GetSelectedFolderPath();
    if (FolderPath.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No folder selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FBPProjectAnalysis Analysis = UBlueprintAnalyzerLibrary::AnalyzeFolder(FolderPath);

    FString Message;
    Message += FString::Printf(TEXT("Project Analysis: %s\n\n"), *Analysis.FolderPath);
    Message += FString::Printf(TEXT("Blueprints: %d\n"), Analysis.BlueprintsAnalyzed);
    Message += FString::Printf(TEXT("Total nodes: %d\n"), Analysis.TotalNodes);
    Message += FString::Printf(TEXT("Average score: %.1f/100\n"), Analysis.AveragePerformanceScore);
    Message += FString::Printf(TEXT("Dependencies: %d\n"), Analysis.Dependencies.Num());
    Message += FString::Printf(TEXT("Circular chains: %d"), Analysis.CircularDependencyChains.Num());

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
}

void FBlueprintAnalyzerMenuExtension::ExecuteExportProjectToJSON()
{
    const FString FolderPath = GetSelectedFolderPath();
    if (FolderPath.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No folder selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FString SavePath = ShowSaveFileDialog(TEXT("ProjectAnalysis.json"), TEXT("JSON Files (*.json)|*.json"));
    if (SavePath.IsEmpty()) return;

    FBPProjectAnalysis Analysis = UBlueprintAnalyzerLibrary::AnalyzeFolder(FolderPath);
    const FString Content = UBlueprintAnalyzerLibrary::ExportProjectAnalysisToJSON(Analysis);
    const bool bSuccess = FFileHelper::SaveStringToFile(Content, *SavePath);

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(bSuccess
        ? FString::Printf(TEXT("Project analysis exported to: %s"), *SavePath)
        : TEXT("Failed to export project analysis.")));
}

void FBlueprintAnalyzerMenuExtension::ExecuteExportProjectToLLMText()
{
    const FString FolderPath = GetSelectedFolderPath();
    if (FolderPath.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("No folder selected."), FText::FromString("Blueprint Analyzer"));
        return;
    }

    FString SavePath = ShowSaveFileDialog(TEXT("ProjectAnalysis_LLM.txt"), TEXT("Text Files (*.txt)|*.txt"));
    if (SavePath.IsEmpty()) return;

    FBPProjectAnalysis Analysis = UBlueprintAnalyzerLibrary::AnalyzeFolder(FolderPath);
    const FString Content = UBlueprintAnalyzerLibrary::ExportProjectAnalysisToLLMText(Analysis);
    const bool bSuccess = FFileHelper::SaveStringToFile(Content, *SavePath);

    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(bSuccess
        ? FString::Printf(TEXT("LLM-friendly project analysis exported to: %s"), *SavePath)
        : TEXT("Failed to export project analysis.")));
}

UBlueprint* FBlueprintAnalyzerMenuExtension::GetSelectedBlueprint()
{
    FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
    TArray<FAssetData> SelectedAssets;
    ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);
    
    UE_LOG(LogTemp, Warning, TEXT("BlueprintAnalyzer: Selected assets count: %d"), SelectedAssets.Num());
    
    for (const FAssetData& AssetData : SelectedAssets)
    {
        UE_LOG(LogTemp, Warning, TEXT("BlueprintAnalyzer: Asset: %s, Class: %s"), *AssetData.AssetName.ToString(), *AssetData.AssetClassPath.ToString());
        
        if (AssetData.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
        {
            UE_LOG(LogTemp, Warning, TEXT("BlueprintAnalyzer: Found Blueprint: %s"), *AssetData.AssetName.ToString());
            return Cast<UBlueprint>(AssetData.GetAsset());
        }
        
        if (AssetData.AssetClassPath.ToString().Contains(TEXT("Blueprint")))
        {
            UE_LOG(LogTemp, Warning, TEXT("BlueprintAnalyzer: Found Blueprint variant: %s"), *AssetData.AssetName.ToString());
            if (UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
            {
                return Blueprint;
            }
        }
    }
    
    return nullptr;
}

bool FBlueprintAnalyzerMenuExtension::IsWidgetBlueprint(UBlueprint* Blueprint)
{
    if (!Blueprint || !Blueprint->GeneratedClass)
    {
        return false;
    }
    
    return Blueprint->GeneratedClass->IsChildOf<UUserWidget>();
}

FString FBlueprintAnalyzerMenuExtension::ShowSaveFileDialog(const FString& DefaultFilename, const FString& FileTypes)
{
    TArray<FString> SaveFilenames;
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    
    if (DesktopPlatform)
    {
        TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().GetActiveTopLevelWindow();
        void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid()) ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;
        
        bool bSaved = DesktopPlatform->SaveFileDialog(
            ParentWindowHandle,
            TEXT("Save Analysis"),
            FPaths::ProjectSavedDir(),
            DefaultFilename,
            FileTypes,
            EFileDialogFlags::None,
            SaveFilenames
        );
        
        if (bSaved && SaveFilenames.Num() > 0)
        {
            return SaveFilenames[0];
        }
    }
    
    return FString();
}