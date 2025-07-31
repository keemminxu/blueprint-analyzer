// Copyright (c) 2025 keemminxu. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Blueprint.h"
#include "Blueprint/UserWidget.h"
#include "WidgetBlueprint.h"
#include "K2Node.h"
#include "BlueprintAnalyzerLibrary.generated.h"

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBlueprintNodeInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString NodeType;

    UPROPERTY(BlueprintReadOnly)
    FString NodeName;

    UPROPERTY(BlueprintReadOnly)
    FString FunctionName;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> InputPins;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> OutputPins;

    UPROPERTY(BlueprintReadOnly)
    FString NodeGuid;

    FBlueprintNodeInfo()
    {
        NodeType = TEXT("");
        NodeName = TEXT("");
        FunctionName = TEXT("");
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBlueprintConnectionInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString FromNodeGuid;

    UPROPERTY(BlueprintReadOnly)
    FString FromPinName;

    UPROPERTY(BlueprintReadOnly)
    FString ToNodeGuid;

    UPROPERTY(BlueprintReadOnly)
    FString ToPinName;

    FBlueprintConnectionInfo()
    {
        FromNodeGuid = TEXT("");
        FromPinName = TEXT("");
        ToNodeGuid = TEXT("");
        ToPinName = TEXT("");
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBlueprintAnalysisResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString BlueprintName;

    UPROPERTY(BlueprintReadOnly)
    TArray<FBlueprintNodeInfo> Nodes;

    UPROPERTY(BlueprintReadOnly)
    TArray<FBlueprintConnectionInfo> Connections;

    UPROPERTY(BlueprintReadOnly)
    FString AnalysisTimestamp;

    FBlueprintAnalysisResult()
    {
        BlueprintName = TEXT("");
        AnalysisTimestamp = TEXT("");
    }
};

// Widget Blueprint Analysis Structures
UENUM(BlueprintType)
enum class EWidgetOptimizationSeverity : uint8
{
    Info,
    Warning,
    Critical
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FWidgetOptimizationIssue
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString IssueType;

    UPROPERTY(BlueprintReadOnly)
    FString Description;

    UPROPERTY(BlueprintReadOnly)
    FString Recommendation;

    UPROPERTY(BlueprintReadOnly)
    EWidgetOptimizationSeverity Severity;

    UPROPERTY(BlueprintReadOnly)
    FString WidgetPath;

    FWidgetOptimizationIssue()
    {
        IssueType = TEXT("");
        Description = TEXT("");
        Recommendation = TEXT("");
        Severity = EWidgetOptimizationSeverity::Info;
        WidgetPath = TEXT("");
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FWidgetHierarchyInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString WidgetName;

    UPROPERTY(BlueprintReadOnly)
    FString WidgetType;

    UPROPERTY(BlueprintReadOnly)
    int32 Depth;

    UPROPERTY(BlueprintReadOnly)
    int32 ChildrenCount;

    UPROPERTY(BlueprintReadOnly)
    bool bHasBindings;

    UPROPERTY(BlueprintReadOnly)
    TArray<FString> BoundProperties;

    FWidgetHierarchyInfo()
    {
        WidgetName = TEXT("");
        WidgetType = TEXT("");
        Depth = 0;
        ChildrenCount = 0;
        bHasBindings = false;
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FWidgetOptimizationReport
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString WidgetBlueprintName;

    UPROPERTY(BlueprintReadOnly)
    FString AnalysisTimestamp;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalWidgets;

    UPROPERTY(BlueprintReadOnly)
    int32 MaxDepth;

    UPROPERTY(BlueprintReadOnly)
    int32 TotalBindings;

    UPROPERTY(BlueprintReadOnly)
    TArray<FWidgetHierarchyInfo> WidgetHierarchy;

    UPROPERTY(BlueprintReadOnly)
    TArray<FWidgetOptimizationIssue> OptimizationIssues;

    UPROPERTY(BlueprintReadOnly)
    float EstimatedMemoryUsage;

    UPROPERTY(BlueprintReadOnly)
    int32 OptimizationScore;

    FWidgetOptimizationReport()
    {
        WidgetBlueprintName = TEXT("");
        AnalysisTimestamp = TEXT("");
        TotalWidgets = 0;
        MaxDepth = 0;
        TotalBindings = 0;
        EstimatedMemoryUsage = 0.0f;
        OptimizationScore = 100;
    }
};

UCLASS()
class BLUEPRINTANALYZER_API UBlueprintAnalyzerLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Original Blueprint Analysis Functions
    UFUNCTION(BlueprintCallable, Category = "Blueprint Analyzer", meta=(CallInEditor="true"))
    static FBlueprintAnalysisResult AnalyzeBlueprint(UBlueprint* Blueprint);

    UFUNCTION(BlueprintCallable, Category = "Blueprint Analyzer", meta=(CallInEditor="true"))
    static FString ExportToJSON(const FBlueprintAnalysisResult& AnalysisResult);

    UFUNCTION(BlueprintCallable, Category = "Blueprint Analyzer", meta=(CallInEditor="true"))
    static FString ExportToLLMText(const FBlueprintAnalysisResult& AnalysisResult);

    UFUNCTION(BlueprintCallable, Category = "Blueprint Analyzer", meta=(CallInEditor="true"))
    static bool SaveAnalysisToFile(const FBlueprintAnalysisResult& AnalysisResult, const FString& FilePath, const FString& Format = TEXT("JSON"));

    // New Widget Blueprint Analysis Functions
    UFUNCTION(BlueprintCallable, Category = "Widget Analyzer", meta=(CallInEditor="true"))
    static FWidgetOptimizationReport AnalyzeWidgetBlueprint(UBlueprint* WidgetBlueprint);

    UFUNCTION(BlueprintCallable, Category = "Widget Analyzer", meta=(CallInEditor="true"))
    static FString ExportWidgetAnalysisToJSON(const FWidgetOptimizationReport& Report);

    UFUNCTION(BlueprintCallable, Category = "Widget Analyzer", meta=(CallInEditor="true"))
    static FString ExportWidgetAnalysisToLLMText(const FWidgetOptimizationReport& Report);

    UFUNCTION(BlueprintCallable, Category = "Widget Analyzer", meta=(CallInEditor="true"))
    static bool SaveWidgetAnalysisToFile(const FWidgetOptimizationReport& Report, const FString& FilePath, const FString& Format = TEXT("JSON"));

    UFUNCTION(BlueprintCallable, Category = "Widget Analyzer", meta=(CallInEditor="true"))
    static FString GenerateOptimizedWidgetCode(const FWidgetOptimizationReport& Report);

private:
    // Original Blueprint Analysis Helper Functions
    static FBlueprintNodeInfo ExtractNodeInfo(UK2Node* Node);
    static TArray<FBlueprintConnectionInfo> ExtractConnections(UEdGraph* Graph);
    static FString GetNodeTypeName(UK2Node* Node);

    // Widget Analysis Helper Functions
    static void AnalyzeWidgetHierarchy(UWidget* Widget, TArray<FWidgetHierarchyInfo>& OutHierarchy, int32 Depth = 0);
    static void CheckForOptimizationIssues(const TArray<FWidgetHierarchyInfo>& Hierarchy, TArray<FWidgetOptimizationIssue>& OutIssues);
    static int32 CalculateOptimizationScore(const TArray<FWidgetOptimizationIssue>& Issues, int32 TotalWidgets);
    static float EstimateMemoryUsage(const TArray<FWidgetHierarchyInfo>& Hierarchy);
    static FString GetWidgetTypeName(UWidget* Widget);
    static bool HasBindings(UWidget* Widget);
    static TArray<FString> GetBoundProperties(UWidget* Widget);
};
