// Copyright (c) 2025 keemminxu. All Rights Reserved.

#include "BlueprintAnalyzerMenuExtension.h"
#include "BlueprintAnalyzerLibrary.h"

#define LOCTEXT_NAMESPACE "FBlueprintAnalyzerModule"
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

    UToolMenu* ContentBrowserAssetMenu = ToolMenus->ExtendMenu("ContentBrowser.AssetContextMenu.Blueprint");
    if (ContentBrowserAssetMenu)
    {
        FToolMenuSection& Section = ContentBrowserAssetMenu->FindOrAddSection("GetAssetActions");
        Section.AddSubMenu(
            "BlueprintAnalyzer",
            LOCTEXT("BlueprintAnalyzerSubMenu", "Blueprint Analyzer"),
            LOCTEXT("BlueprintAnalyzerSubMenuTooltip", "Blueprint analysis and optimization tools"),
            FNewToolMenuDelegate::CreateLambda([](UToolMenu* SubMenu)
            {
                FToolMenuSection& SubSection = SubMenu->AddSection("BlueprintAnalyzerActions", LOCTEXT("BlueprintAnalyzerActions", "Analysis Actions"));
                
                // Regular Blueprint Analysis
                SubSection.AddMenuEntry(
                    "AnalyzeBlueprint",
                    LOCTEXT("AnalyzeBlueprint", "Analyze Blueprint"),
                    LOCTEXT("AnalyzeBlueprintTooltip", "Analyze the selected blueprint structure"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeBlueprint))
                );
                
                SubSection.AddMenuEntry(
                    "ExportToJSON",
                    LOCTEXT("ExportToJSON", "Export to JSON"),
                    LOCTEXT("ExportToJSONTooltip", "Export blueprint analysis to JSON format"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportToJSON))
                );
                
                SubSection.AddMenuEntry(
                    "ExportToLLMText",
                    LOCTEXT("ExportToLLMText", "Export to LLM Text"),
                    LOCTEXT("ExportToLLMTextTooltip", "Export blueprint analysis to LLM-friendly text format"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportToLLMText))
                );

                // Widget Blueprint Optimization
                FToolMenuSection& WidgetSection = SubMenu->AddSection("WidgetAnalyzerActions", LOCTEXT("WidgetAnalyzerActions", "Widget Optimization"));
                
                WidgetSection.AddMenuEntry(
                    "AnalyzeWidgetBlueprint",
                    LOCTEXT("AnalyzeWidgetBlueprint", "Analyze Widget Blueprint"),
                    LOCTEXT("AnalyzeWidgetBlueprintTooltip", "Analyze widget blueprint for optimization opportunities"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeWidgetBlueprint))
                );
                
                WidgetSection.AddMenuEntry(
                    "ExportWidgetToJSON",
                    LOCTEXT("ExportWidgetToJSON", "Export Widget Analysis to JSON"),
                    LOCTEXT("ExportWidgetToJSONTooltip", "Export widget optimization report to JSON format"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportWidgetToJSON))
                );
                
                WidgetSection.AddMenuEntry(
                    "ExportWidgetToLLMText",
                    LOCTEXT("ExportWidgetToLLMText", "Export Widget Analysis to LLM Text"),
                    LOCTEXT("ExportWidgetToLLMTextTooltip", "Export widget optimization report to LLM-friendly text format"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteExportWidgetToLLMText))
                );
                
                //WidgetSection.AddMenuEntry(
                //    "GenerateOptimizedCode",
                //    LOCTEXT("GenerateOptimizedCode", "Generate Optimized C++ Code"),
                //    LOCTEXT("GenerateOptimizedCodeTooltip", "Generate optimized C++ code based on widget analysis"),
                //    FSlateIcon(),
                //    FUIAction(FExecuteAction::CreateStatic(&FBlueprintAnalyzerMenuExtension::ExecuteGenerateOptimizedCode))
                //);
            })
        );
    }
}

void FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeBlueprint()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBlueprintSelected", "No blueprint selected."));
        return;
    }

    FBlueprintAnalysisResult AnalysisResult = UBlueprintAnalyzerLibrary::AnalyzeBlueprint(SelectedBlueprint);
    
    FString Message = FString::Printf(TEXT("Blueprint '%s' analyzed successfully!\n\nNodes: %d\nConnections: %d"), 
        *AnalysisResult.BlueprintName, 
        AnalysisResult.Nodes.Num(), 
        AnalysisResult.Connections.Num());
        
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message));
}

void FBlueprintAnalyzerMenuExtension::ExecuteExportToJSON()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBlueprintSelected", "No blueprint selected."));
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
            FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ExportFailed", "Failed to export analysis."));
        }
    }
}

void FBlueprintAnalyzerMenuExtension::ExecuteExportToLLMText()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBlueprintSelected", "No blueprint selected."));
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
            FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ExportFailed", "Failed to export analysis."));
        }
    }
}

void FBlueprintAnalyzerMenuExtension::ExecuteAnalyzeWidgetBlueprint()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBlueprintSelected", "No blueprint selected."));
        return;
    }

    if (!IsWidgetBlueprint(SelectedBlueprint))
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotWidgetBlueprint", "Selected blueprint is not a Widget Blueprint."));
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
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBlueprintSelected", "No blueprint selected."));
        return;
    }

    if (!IsWidgetBlueprint(SelectedBlueprint))
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotWidgetBlueprint", "Selected blueprint is not a Widget Blueprint."));
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
            FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ExportFailed", "Failed to export widget analysis."));
        }
    }
}

void FBlueprintAnalyzerMenuExtension::ExecuteExportWidgetToLLMText()
{
    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
    if (!SelectedBlueprint)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBlueprintSelected", "No blueprint selected."));
        return;
    }

    if (!IsWidgetBlueprint(SelectedBlueprint))
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotWidgetBlueprint", "Selected blueprint is not a Widget Blueprint."));
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
            FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ExportFailed", "Failed to export widget analysis."));
        }
    }
}

//void FBlueprintAnalyzerMenuExtension::ExecuteGenerateOptimizedCode()
//{
//    UBlueprint* SelectedBlueprint = GetSelectedBlueprint();
//    if (!SelectedBlueprint)
//    {
//        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NoBlueprintSelected", "No blueprint selected."));
//        return;
//    }
//
//    if (!IsWidgetBlueprint(SelectedBlueprint))
//    {
//        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("NotWidgetBlueprint", "Selected blueprint is not a Widget Blueprint."));
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

#undef LOCTEXT_NAMESPACE