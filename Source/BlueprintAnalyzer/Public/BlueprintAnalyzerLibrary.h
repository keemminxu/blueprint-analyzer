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

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString NodeType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString NodeName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString FunctionName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FString> InputPins;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FString> OutputPins;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString NodeGuid;

    // Phase 1: name of the graph this node belongs to (EventGraph / function / macro / etc.)
    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString GraphName;

    // Phase 5: literal/default values on disconnected input pins (e.g. `Print String("Hello")`)
    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FString> LiteralValues;

    // Phase 5: comment box title that wraps this node, if any
    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString CommentGroup;

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

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString FromNodeGuid;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString FromPinName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ToNodeGuid;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ToPinName;

    FBlueprintConnectionInfo()
    {
        FromNodeGuid = TEXT("");
        FromPinName = TEXT("");
        ToNodeGuid = TEXT("");
        ToPinName = TEXT("");
    }
};

// ============================================================
// Phase 1: Blueprint Metadata Structures
// ============================================================

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBPVariableInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString VariableName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString VariableType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString DefaultValue;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString Category;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString Tooltip;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bEditable;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bReplicated;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bExposeOnSpawn;

    FBPVariableInfo()
    {
        bEditable = false;
        bReplicated = false;
        bExposeOnSpawn = false;
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBPFunctionParam
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ParamName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ParamType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bIsReference;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bIsConst;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bIsReturn;

    FBPFunctionParam()
    {
        bIsReference = false;
        bIsConst = false;
        bIsReturn = false;
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBPFunctionSignature
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString FunctionName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FBPFunctionParam> Parameters;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ReturnType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString AccessSpecifier;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString Category;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bPure;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bStatic;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bConst;

    FBPFunctionSignature()
    {
        bPure = false;
        bStatic = false;
        bConst = false;
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBPComponentInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ComponentName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ComponentType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ParentComponentName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString AttachSocketName;

    FBPComponentInfo()
    {
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBPEventDispatcherInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString DispatcherName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FBPFunctionParam> Parameters;

    FBPEventDispatcherInfo()
    {
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBPAnalyzerMetadata
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString BlueprintType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ParentClass;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FString> ImplementedInterfaces;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FBPVariableInfo> Variables;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FBPFunctionSignature> CustomFunctions;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FBPComponentInfo> Components;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FBPEventDispatcherInfo> EventDispatchers;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FString> MacroNames;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FString> TimelineNames;

    FBPAnalyzerMetadata()
    {
    }
};

// ============================================================
// Phase 2: Execution Flow Structures
// ============================================================

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FExecutionStep
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString NodeGuid;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString NodeType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString Summary;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString BranchLabel;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 Depth;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bIsTerminator;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bIsLatent;

    FExecutionStep()
    {
        Depth = 0;
        bIsTerminator = false;
        bIsLatent = false;
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FExecutionPath
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString EntryPointName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString EntryNodeGuid;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString GraphName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FExecutionStep> Steps;

    FExecutionPath()
    {
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBlueprintAnalysisResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString BlueprintName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FBPAnalyzerMetadata Metadata;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FBlueprintNodeInfo> Nodes;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FBlueprintConnectionInfo> Connections;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FExecutionPath> ExecutionPaths;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString AnalysisTimestamp;

    FBlueprintAnalysisResult()
    {
        BlueprintName = TEXT("");
        AnalysisTimestamp = TEXT("");
    }
};

// ============================================================
// Phase 3: Blueprint Performance Analysis Structures
// ============================================================

UENUM(BlueprintType)
enum class EBPPerformanceSeverity : uint8
{
    Info,
    Warning,
    Critical
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBPPerformanceIssue
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString IssueType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString Description;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString Recommendation;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    EBPPerformanceSeverity Severity;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString NodeGuid;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString GraphName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 Deduction;

    FBPPerformanceIssue()
    {
        Severity = EBPPerformanceSeverity::Info;
        Deduction = 0;
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBPPerformanceReport
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString BlueprintName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString AnalysisTimestamp;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 TotalNodes;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 EventCount;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 CastCount;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 TickNodeCount;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 BeginPlayNodeCount;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FBPPerformanceIssue> Issues;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 PerformanceScore;

    FBPPerformanceReport()
    {
        TotalNodes = 0;
        EventCount = 0;
        CastCount = 0;
        TickNodeCount = 0;
        BeginPlayNodeCount = 0;
        PerformanceScore = 100;
    }
};

// ============================================================
// Phase 4: Project Dependency Analysis Structures
// ============================================================

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBPDependency
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ReferencingBlueprint;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ReferencedClass;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString ReferenceType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString NodeGuid;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString GraphName;

    FBPDependency()
    {
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBPBlueprintSummary
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString BlueprintName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString BlueprintPath;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString BlueprintType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 NodeCount;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 PerformanceScore;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 CriticalIssues;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 EstimatedTokenCount;

    FBPBlueprintSummary()
    {
        NodeCount = 0;
        PerformanceScore = 100;
        CriticalIssues = 0;
        EstimatedTokenCount = 0;
    }
};

USTRUCT(BlueprintType)
struct BLUEPRINTANALYZER_API FBPProjectAnalysis
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString FolderPath;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString AnalysisTimestamp;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 BlueprintsAnalyzed;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 TotalNodes;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    float AveragePerformanceScore;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FBPBlueprintSummary> Summaries;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FBPDependency> Dependencies;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FString> CircularDependencyChains;

    FBPProjectAnalysis()
    {
        BlueprintsAnalyzed = 0;
        TotalNodes = 0;
        AveragePerformanceScore = 100.0f;
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

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString IssueType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString Description;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString Recommendation;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    EWidgetOptimizationSeverity Severity;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
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

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString WidgetName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString WidgetType;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 Depth;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 ChildrenCount;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    bool bHasBindings;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
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

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString WidgetBlueprintName;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    FString AnalysisTimestamp;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 TotalWidgets;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 MaxDepth;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    int32 TotalBindings;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FWidgetHierarchyInfo> WidgetHierarchy;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    TArray<FWidgetOptimizationIssue> OptimizationIssues;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
    float EstimatedMemoryUsage;

    UPROPERTY(BlueprintReadOnly, Category = "BlueprintAnalyzer")
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
    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static FBlueprintAnalysisResult AnalyzeBlueprint(UBlueprint* Blueprint);

    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static FString ExportToJSON(const FBlueprintAnalysisResult& AnalysisResult);

    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static FString ExportToLLMText(const FBlueprintAnalysisResult& AnalysisResult);

    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static bool SaveAnalysisToFile(const FBlueprintAnalysisResult& AnalysisResult, const FString& FilePath, const FString& Format = TEXT("JSON"));

    // Phase 3: Blueprint Performance Analysis Functions
    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static FBPPerformanceReport AnalyzeBlueprintPerformance(UBlueprint* Blueprint);

    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static FString ExportPerformanceReportToJSON(const FBPPerformanceReport& Report);

    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static FString ExportPerformanceReportToLLMText(const FBPPerformanceReport& Report);

    // Phase 4: Project Dependency Analysis
    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static FBPProjectAnalysis AnalyzeFolder(const FString& FolderPath);

    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static FString ExportProjectAnalysisToJSON(const FBPProjectAnalysis& Analysis);

    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static FString ExportProjectAnalysisToLLMText(const FBPProjectAnalysis& Analysis);

    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static TArray<FBPDependency> ExtractBlueprintDependencies(UBlueprint* Blueprint);

    UFUNCTION(BlueprintCallable, Category = "BlueprintAnalyzer", meta=(CallInEditor="true"))
    static int32 EstimateTokenCount(const FString& Text);

    // New Widget Blueprint Analysis Functions
    UFUNCTION(BlueprintCallable, Category = "WidgetAnalyzer", meta=(CallInEditor="true"))
    static FWidgetOptimizationReport AnalyzeWidgetBlueprint(UBlueprint* WidgetBlueprint);

    UFUNCTION(BlueprintCallable, Category = "WidgetAnalyzer", meta=(CallInEditor="true"))
    static FString ExportWidgetAnalysisToJSON(const FWidgetOptimizationReport& Report);

    UFUNCTION(BlueprintCallable, Category = "WidgetAnalyzer", meta=(CallInEditor="true"))
    static FString ExportWidgetAnalysisToLLMText(const FWidgetOptimizationReport& Report);

    UFUNCTION(BlueprintCallable, Category = "WidgetAnalyzer", meta=(CallInEditor="true"))
    static bool SaveWidgetAnalysisToFile(const FWidgetOptimizationReport& Report, const FString& FilePath, const FString& Format = TEXT("JSON"));

    UFUNCTION(BlueprintCallable, Category = "WidgetAnalyzer", meta=(CallInEditor="true"))
    static FString GenerateOptimizedWidgetCode(const FWidgetOptimizationReport& Report);

private:
    // Original Blueprint Analysis Helper Functions
    static FBlueprintNodeInfo ExtractNodeInfo(UK2Node* Node);
    static TArray<FBlueprintConnectionInfo> ExtractConnections(UEdGraph* Graph);
    static FString GetNodeTypeName(UK2Node* Node);

    // Phase 1: Metadata Extraction Helper Functions
    static FBPAnalyzerMetadata ExtractMetadata(UBlueprint* Blueprint);
    static FString GetBlueprintTypeString(UBlueprint* Blueprint);
    static FString GetPinTypeString(const struct FEdGraphPinType& PinType);
    static FString GetPropertyTypeString(const class FProperty* Property);
    static FString ExtractLiteralFromPin(const class UEdGraphPin* Pin);
    static FString FindCommentGroupForNode(const class UEdGraphNode* Node, const class UEdGraph* Graph);

    // Phase 2: Execution Flow Helpers
    static TArray<FExecutionPath> TraceExecutionPaths(UBlueprint* Blueprint);
    static void TraceFromNode(
        class UK2Node* StartNode,
        int32 Depth,
        const FString& BranchLabel,
        TArray<FExecutionStep>& OutSteps,
        TSet<FGuid>& VisitedNodes);
    static FString GetExecutionStepSummary(class UK2Node* Node);

    // Widget Analysis Helper Functions
    static void AnalyzeWidgetHierarchy(UWidget* Widget, TArray<FWidgetHierarchyInfo>& OutHierarchy, int32 Depth = 0);
    static void CheckForOptimizationIssues(const TArray<FWidgetHierarchyInfo>& Hierarchy, TArray<FWidgetOptimizationIssue>& OutIssues);
    static int32 CalculateOptimizationScore(const TArray<FWidgetOptimizationIssue>& Issues, int32 TotalWidgets);
    static float EstimateMemoryUsage(const TArray<FWidgetHierarchyInfo>& Hierarchy);
    static FString GetWidgetTypeName(UWidget* Widget);
    static bool HasBindings(UWidget* Widget);
    static TArray<FString> GetBoundProperties(UWidget* Widget);
};
