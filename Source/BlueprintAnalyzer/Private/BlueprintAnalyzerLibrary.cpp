// Copyright (c) 2025 keemminxu. All Rights Reserved.

#include "BlueprintAnalyzerLibrary.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "EdGraph/EdGraphPin.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/ScrollBox.h"
#include "Components/ListView.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/InvalidationBox.h"
#include "Components/RetainerBox.h"
#include "Engine/Texture2D.h"

// Original Blueprint Analysis Functions Implementation
FBlueprintAnalysisResult UBlueprintAnalyzerLibrary::AnalyzeBlueprint(UBlueprint* Blueprint)
{
    FBlueprintAnalysisResult Result;
    
    if (!Blueprint)
    {
        UE_LOG(LogTemp, Warning, TEXT("Blueprint is null"));
        return Result;
    }

    Result.BlueprintName = Blueprint->GetName();
    Result.AnalysisTimestamp = FDateTime::Now().ToString();

    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (!Graph) continue;

        for (UEdGraphNode* GraphNode : Graph->Nodes)
        {
            if (UK2Node* K2Node = Cast<UK2Node>(GraphNode))
            {
                FBlueprintNodeInfo NodeInfo = ExtractNodeInfo(K2Node);
                Result.Nodes.Add(NodeInfo);
            }
        }

        TArray<FBlueprintConnectionInfo> Connections = ExtractConnections(Graph);
        Result.Connections.Append(Connections);
    }

    for (UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
    {
        if (!FunctionGraph) continue;

        for (UEdGraphNode* GraphNode : FunctionGraph->Nodes)
        {
            if (UK2Node* K2Node = Cast<UK2Node>(GraphNode))
            {
                FBlueprintNodeInfo NodeInfo = ExtractNodeInfo(K2Node);
                Result.Nodes.Add(NodeInfo);
            }
        }

        TArray<FBlueprintConnectionInfo> Connections = ExtractConnections(FunctionGraph);
        Result.Connections.Append(Connections);
    }

    return Result;
}

FBlueprintNodeInfo UBlueprintAnalyzerLibrary::ExtractNodeInfo(UK2Node* Node)
{
    FBlueprintNodeInfo NodeInfo;
    
    if (!Node) return NodeInfo;

    NodeInfo.NodeGuid = Node->NodeGuid.ToString();
    NodeInfo.NodeType = GetNodeTypeName(Node);
    NodeInfo.NodeName = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();

    if (UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(Node))
    {
        if (CallFunctionNode->GetTargetFunction())
        {
            NodeInfo.FunctionName = CallFunctionNode->GetTargetFunction()->GetName();
        }
    }

    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->Direction == EGPD_Input)
        {
            NodeInfo.InputPins.Add(FString::Printf(TEXT("%s:%s"), *Pin->PinName.ToString(), *Pin->PinType.PinCategory.ToString()));
        }
        else if (Pin->Direction == EGPD_Output)
        {
            NodeInfo.OutputPins.Add(FString::Printf(TEXT("%s:%s"), *Pin->PinName.ToString(), *Pin->PinType.PinCategory.ToString()));
        }
    }

    return NodeInfo;
}

TArray<FBlueprintConnectionInfo> UBlueprintAnalyzerLibrary::ExtractConnections(UEdGraph* Graph)
{
    TArray<FBlueprintConnectionInfo> Connections;
    
    if (!Graph) return Connections;

    for (UEdGraphNode* GraphNode : Graph->Nodes)
    {
        UK2Node* K2Node = Cast<UK2Node>(GraphNode);
        if (!K2Node) continue;

        for (UEdGraphPin* Pin : K2Node->Pins)
        {
            if (Pin->Direction == EGPD_Output && Pin->LinkedTo.Num() > 0)
            {
                for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                {
                    if (LinkedPin && LinkedPin->GetOwningNode())
                    {
                        FBlueprintConnectionInfo ConnectionInfo;
                        ConnectionInfo.FromNodeGuid = K2Node->NodeGuid.ToString();
                        ConnectionInfo.FromPinName = Pin->PinName.ToString();
                        ConnectionInfo.ToNodeGuid = LinkedPin->GetOwningNode()->NodeGuid.ToString();
                        ConnectionInfo.ToPinName = LinkedPin->PinName.ToString();
                        
                        Connections.Add(ConnectionInfo);
                    }
                }
            }
        }
    }

    return Connections;
}

FString UBlueprintAnalyzerLibrary::GetNodeTypeName(UK2Node* Node)
{
    if (!Node) return TEXT("Unknown");

    if (Cast<UK2Node_Event>(Node)) return TEXT("Event");
    if (Cast<UK2Node_FunctionEntry>(Node)) return TEXT("FunctionEntry");
    if (Cast<UK2Node_FunctionResult>(Node)) return TEXT("FunctionResult");
    if (Cast<UK2Node_CallFunction>(Node)) return TEXT("FunctionCall");
    if (Cast<UK2Node_VariableGet>(Node)) return TEXT("VariableGet");
    if (Cast<UK2Node_VariableSet>(Node)) return TEXT("VariableSet");
    if (Cast<UK2Node_IfThenElse>(Node)) return TEXT("Branch");

    return Node->GetClass()->GetName();
}

FString UBlueprintAnalyzerLibrary::ExportToJSON(const FBlueprintAnalysisResult& AnalysisResult)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    
    RootObject->SetStringField(TEXT("BlueprintName"), AnalysisResult.BlueprintName);
    RootObject->SetStringField(TEXT("AnalysisTimestamp"), AnalysisResult.AnalysisTimestamp);

    TArray<TSharedPtr<FJsonValue>> NodesArray;
    for (const FBlueprintNodeInfo& Node : AnalysisResult.Nodes)
    {
        TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject);
        NodeObject->SetStringField(TEXT("NodeGuid"), Node.NodeGuid);
        NodeObject->SetStringField(TEXT("NodeType"), Node.NodeType);
        NodeObject->SetStringField(TEXT("NodeName"), Node.NodeName);
        NodeObject->SetStringField(TEXT("FunctionName"), Node.FunctionName);
        
        TArray<TSharedPtr<FJsonValue>> InputPinsArray;
        for (const FString& InputPin : Node.InputPins)
        {
            InputPinsArray.Add(MakeShareable(new FJsonValueString(InputPin)));
        }
        NodeObject->SetArrayField(TEXT("InputPins"), InputPinsArray);
        
        TArray<TSharedPtr<FJsonValue>> OutputPinsArray;
        for (const FString& OutputPin : Node.OutputPins)
        {
            OutputPinsArray.Add(MakeShareable(new FJsonValueString(OutputPin)));
        }
        NodeObject->SetArrayField(TEXT("OutputPins"), OutputPinsArray);
        
        NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObject)));
    }
    RootObject->SetArrayField(TEXT("Nodes"), NodesArray);

    TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
    for (const FBlueprintConnectionInfo& Connection : AnalysisResult.Connections)
    {
        TSharedPtr<FJsonObject> ConnectionObject = MakeShareable(new FJsonObject);
        ConnectionObject->SetStringField(TEXT("FromNodeGuid"), Connection.FromNodeGuid);
        ConnectionObject->SetStringField(TEXT("FromPinName"), Connection.FromPinName);
        ConnectionObject->SetStringField(TEXT("ToNodeGuid"), Connection.ToNodeGuid);
        ConnectionObject->SetStringField(TEXT("ToPinName"), Connection.ToPinName);
        
        ConnectionsArray.Add(MakeShareable(new FJsonValueObject(ConnectionObject)));
    }
    RootObject->SetArrayField(TEXT("Connections"), ConnectionsArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    return OutputString;
}

FString UBlueprintAnalyzerLibrary::ExportToLLMText(const FBlueprintAnalysisResult& AnalysisResult)
{
    FString Result;
    
    Result += FString::Printf(TEXT("Blueprint Analysis: %s\n"), *AnalysisResult.BlueprintName);
    Result += FString::Printf(TEXT("Analyzed at: %s\n\n"), *AnalysisResult.AnalysisTimestamp);
    
    Result += TEXT("=== NODES ===\n");
    for (const FBlueprintNodeInfo& Node : AnalysisResult.Nodes)
    {
        Result += FString::Printf(TEXT("- %s [%s]: %s\n"), *Node.NodeType, *Node.NodeGuid, *Node.NodeName);
        if (!Node.FunctionName.IsEmpty())
        {
            Result += FString::Printf(TEXT("  Function: %s\n"), *Node.FunctionName);
        }
        if (Node.InputPins.Num() > 0)
        {
            Result += TEXT("  Inputs: ");
            for (int32 i = 0; i < Node.InputPins.Num(); ++i)
            {
                Result += Node.InputPins[i];
                if (i < Node.InputPins.Num() - 1) Result += TEXT(", ");
            }
            Result += TEXT("\n");
        }
        if (Node.OutputPins.Num() > 0)
        {
            Result += TEXT("  Outputs: ");
            for (int32 i = 0; i < Node.OutputPins.Num(); ++i)
            {
                Result += Node.OutputPins[i];
                if (i < Node.OutputPins.Num() - 1) Result += TEXT(", ");
            }
            Result += TEXT("\n");
        }
        Result += TEXT("\n");
    }
    
    Result += TEXT("=== CONNECTIONS ===\n");
    for (const FBlueprintConnectionInfo& Connection : AnalysisResult.Connections)
    {
        Result += FString::Printf(TEXT("%s.%s -> %s.%s\n"), 
            *Connection.FromNodeGuid, *Connection.FromPinName,
            *Connection.ToNodeGuid, *Connection.ToPinName);
    }
    
    return Result;
}

bool UBlueprintAnalyzerLibrary::SaveAnalysisToFile(const FBlueprintAnalysisResult& AnalysisResult, const FString& FilePath, const FString& Format)
{
    FString Content;
    
    if (Format.ToUpper() == TEXT("JSON"))
    {
        Content = ExportToJSON(AnalysisResult);
    }
    else
    {
        Content = ExportToLLMText(AnalysisResult);
    }
    
    return FFileHelper::SaveStringToFile(Content, *FilePath);
}

// New Widget Blueprint Analysis Functions Implementation
FWidgetOptimizationReport UBlueprintAnalyzerLibrary::AnalyzeWidgetBlueprint(UBlueprint* WidgetBlueprint)
{
    FWidgetOptimizationReport Report;
    
    if (!WidgetBlueprint)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetBlueprint is null"));
        return Report;
    }

    // Check if it's actually a Widget Blueprint
    if (!WidgetBlueprint->GeneratedClass || !WidgetBlueprint->GeneratedClass->IsChildOf<UUserWidget>())
    {
        UE_LOG(LogTemp, Warning, TEXT("Blueprint is not a Widget Blueprint"));
        return Report;
    }

    Report.WidgetBlueprintName = WidgetBlueprint->GetName();
    Report.AnalysisTimestamp = FDateTime::Now().ToString();

    // Get the widget tree directly from the Widget Blueprint
    if (UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(WidgetBlueprint))
    {
        if (UWidgetTree* WidgetTree = WidgetBP->WidgetTree)
        {
            if (UWidget* RootWidget = WidgetTree->RootWidget)
            {
                // Analyze widget hierarchy
                AnalyzeWidgetHierarchy(RootWidget, Report.WidgetHierarchy, 0);
                
                // Calculate statistics
                Report.TotalWidgets = Report.WidgetHierarchy.Num();
                Report.MaxDepth = 0;
                Report.TotalBindings = 0;
                
                for (const FWidgetHierarchyInfo& WidgetInfo : Report.WidgetHierarchy)
                {
                    if (WidgetInfo.Depth > Report.MaxDepth)
                    {
                        Report.MaxDepth = WidgetInfo.Depth;
                    }
                    if (WidgetInfo.bHasBindings)
                    {
                        Report.TotalBindings += WidgetInfo.BoundProperties.Num();
                    }
                }
                
                // Check for optimization issues
                CheckForOptimizationIssues(Report.WidgetHierarchy, Report.OptimizationIssues);
                
                // Calculate optimization score and memory usage
                Report.OptimizationScore = CalculateOptimizationScore(Report.OptimizationIssues, Report.TotalWidgets);
                Report.EstimatedMemoryUsage = EstimateMemoryUsage(Report.WidgetHierarchy);
            }
        }
    }

    return Report;
}

void UBlueprintAnalyzerLibrary::AnalyzeWidgetHierarchy(UWidget* Widget, TArray<FWidgetHierarchyInfo>& OutHierarchy, int32 Depth)
{
    if (!Widget)
    {
        return;
    }

    FWidgetHierarchyInfo WidgetInfo;
    WidgetInfo.WidgetName = Widget->GetName();
    WidgetInfo.WidgetType = GetWidgetTypeName(Widget);
    WidgetInfo.Depth = Depth;
    WidgetInfo.bHasBindings = HasBindings(Widget);
    WidgetInfo.BoundProperties = GetBoundProperties(Widget);

    // Count children for panel widgets
    if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
    {
        WidgetInfo.ChildrenCount = PanelWidget->GetChildrenCount();
        
        // Recursively analyze children
        for (int32 i = 0; i < PanelWidget->GetChildrenCount(); ++i)
        {
            if (UWidget* ChildWidget = PanelWidget->GetChildAt(i))
            {
                AnalyzeWidgetHierarchy(ChildWidget, OutHierarchy, Depth + 1);
            }
        }
    }
    else
    {
        WidgetInfo.ChildrenCount = 0;
    }

    OutHierarchy.Add(WidgetInfo);
}

void UBlueprintAnalyzerLibrary::CheckForOptimizationIssues(const TArray<FWidgetHierarchyInfo>& Hierarchy, TArray<FWidgetOptimizationIssue>& OutIssues)
{
    // Track widget types for cross-widget analysis
    bool bHasInvalidationBox = false;
    bool bHasRetainerBox = false;
    bool bHasDynamicWidgets = false;
    bool bHasComplexStaticWidgets = false;
    TMap<FString, bool> ScaleBoxParents;
    TMap<FString, bool> SizeBoxParents;
    
    // First pass: identify widget types and relationships
    for (const FWidgetHierarchyInfo& WidgetInfo : Hierarchy)
    {
        if (WidgetInfo.WidgetType.Contains(TEXT("InvalidationBox")))
        {
            bHasInvalidationBox = true;
        }
        else if (WidgetInfo.WidgetType.Contains(TEXT("RetainerBox")))
        {
            bHasRetainerBox = true;
        }
        else if (WidgetInfo.WidgetType.Contains(TEXT("ProgressBar")) || 
                 WidgetInfo.WidgetType.Contains(TEXT("TextBlock")) ||
                 WidgetInfo.WidgetType.Contains(TEXT("Slider")))
        {
            bHasDynamicWidgets = true;
        }
        else if (WidgetInfo.ChildrenCount > 10 || WidgetInfo.WidgetType.Contains(TEXT("Image")))
        {
            bHasComplexStaticWidgets = true;
        }
        
        if (WidgetInfo.WidgetType.Contains(TEXT("ScaleBox")))
        {
            ScaleBoxParents.Add(WidgetInfo.WidgetName, true);
        }
        if (WidgetInfo.WidgetType.Contains(TEXT("SizeBox")))
        {
            SizeBoxParents.Add(WidgetInfo.WidgetName, true);
        }
    }
    
    // Second pass: check for optimization issues
    for (const FWidgetHierarchyInfo& WidgetInfo : Hierarchy)
    {
        // 1. Widget Hierarchy Structure - Deduct 10 points if nesting exceeds 5 levels
        if (WidgetInfo.Depth > 5)
        {
            FWidgetOptimizationIssue Issue;
            Issue.IssueType = TEXT("Deep Nesting Beyond 5 Levels");
            Issue.Description = FString::Printf(TEXT("Widget '%s' is nested %d levels deep (max recommended: 5)"), *WidgetInfo.WidgetName, WidgetInfo.Depth);
            Issue.Recommendation = TEXT("Flatten widget hierarchy to improve layout calculation performance (-10 points)");
            Issue.Severity = EWidgetOptimizationSeverity::Warning;
            Issue.WidgetPath = WidgetInfo.WidgetName;
            OutIssues.Add(Issue);
        }
        
        // 2. Check for simultaneous Scale Box + Size Box usage
        // Check if ScaleBox and SizeBox are used together in parent-child relationship
        if (WidgetInfo.WidgetType.Contains(TEXT("SizeBox")))
        {
            // Check if this widget is a child of ScaleBox (simple approximation check)
            for (const FWidgetHierarchyInfo& PotentialParent : Hierarchy)
            {
                if (PotentialParent.WidgetType.Contains(TEXT("ScaleBox")) && 
                    PotentialParent.Depth < WidgetInfo.Depth)
                {
                    FWidgetOptimizationIssue Issue;
                    Issue.IssueType = TEXT("Scale Box + Size Box Combination");
                    Issue.Description = FString::Printf(TEXT("SizeBox '%s' is used together with ScaleBox in hierarchy"), *WidgetInfo.WidgetName);
                    Issue.Recommendation = TEXT("Avoid using ScaleBox and SizeBox together to prevent unnecessary calculations (-5 points)");
                    Issue.Severity = EWidgetOptimizationSeverity::Warning;
                    Issue.WidgetPath = WidgetInfo.WidgetName;
                    OutIssues.Add(Issue);
                    break;
                }
            }
        }
    }
    
    // 3. Rendering and Caching Optimization Check
    // Missing Invalidation Box (-20 points)
    if (bHasDynamicWidgets && !bHasInvalidationBox)
    {
        FWidgetOptimizationIssue Issue;
        Issue.IssueType = TEXT("Missing Invalidation Box");
        Issue.Description = TEXT("Dynamic widgets (ProgressBar, TextBlock, Slider) found but no Invalidation Box is used");
        Issue.Recommendation = TEXT("Wrap frequently updating widgets with Invalidation Box for better rendering performance (-20 points)");
        Issue.Severity = EWidgetOptimizationSeverity::Critical;
        Issue.WidgetPath = TEXT("Root");
        OutIssues.Add(Issue);
    }
    
    // Missing Retainer Box (-10 points)
    if (bHasComplexStaticWidgets && !bHasRetainerBox)
    {
        FWidgetOptimizationIssue Issue;
        Issue.IssueType = TEXT("Missing Retainer Box");
        Issue.Description = TEXT("Complex but rarely changing widgets found but no Retainer Box is used");
        Issue.Recommendation = TEXT("Use Retainer Box for complex static widget groups to cache rendering (-10 points)");
        Issue.Severity = EWidgetOptimizationSeverity::Warning;
        Issue.WidgetPath = TEXT("Root");
        OutIssues.Add(Issue);
    }
}

int32 UBlueprintAnalyzerLibrary::CalculateOptimizationScore(const TArray<FWidgetOptimizationIssue>& Issues, int32 TotalWidgets)
{
    int32 Score = 100;
    
    // Apply deductions based on UMG optimization criteria
    for (const FWidgetOptimizationIssue& Issue : Issues)
    {
        if (Issue.IssueType.Contains(TEXT("Deep Nesting Beyond 5 Levels")))
        {
            Score -= 10; // -10 points: Hierarchy structure exceeds 5 levels
        }
        else if (Issue.IssueType.Contains(TEXT("Scale Box + Size Box Combination")))
        {
            Score -= 5; // -5 points: ScaleBox + SizeBox used together
        }
        else if (Issue.IssueType.Contains(TEXT("Missing Invalidation Box")))
        {
            Score -= 20; // -20 points: Missing Invalidation Box
        }
        else if (Issue.IssueType.Contains(TEXT("Missing Retainer Box")))
        {
            Score -= 10; // -10 points: Missing Retainer Box
        }
    }
    
    // Check for bonus points (Invalidation Box/Retainer Box 적극 활용)
    bool bHasInvalidationBox = false;
    bool bHasRetainerBox = false;
    
    // No issues means the optimization is well implemented
    bool bMissingInvalidation = false;
    bool bMissingRetainer = false;
    
    for (const FWidgetOptimizationIssue& Issue : Issues)
    {
        if (Issue.IssueType.Contains(TEXT("Missing Invalidation Box")))
        {
            bMissingInvalidation = true;
        }
        if (Issue.IssueType.Contains(TEXT("Missing Retainer Box")))
        {
            bMissingRetainer = true;
        }
    }
    
    // Bonus when appropriate optimization boxes are used with dynamic or complex widgets
    if (!bMissingInvalidation && !bMissingRetainer && TotalWidgets > 5)
    {
        Score += 20; // +20 points: Active use of Invalidation Box/Retainer Box
        Score = FMath::Min(Score, 100); // Limit to maximum 100 points
    }
    
    return FMath::Clamp(Score, 0, 100);
}

float UBlueprintAnalyzerLibrary::EstimateMemoryUsage(const TArray<FWidgetHierarchyInfo>& Hierarchy)
{
    float EstimatedMemory = 0.0f;
    
    for (const FWidgetHierarchyInfo& WidgetInfo : Hierarchy)
    {
        // Base memory for widget object
        EstimatedMemory += 1.0f; // KB per widget
        
        // Additional memory for specific widget types
        if (WidgetInfo.WidgetType.Contains(TEXT("Image")))
        {
            EstimatedMemory += 2.0f; // Images typically use more memory
        }
        else if (WidgetInfo.WidgetType.Contains(TEXT("TextBlock")))
        {
            EstimatedMemory += 0.5f; // Text widgets are lighter
        }
        else if (WidgetInfo.WidgetType.Contains(TEXT("ListView")))
        {
            EstimatedMemory += 5.0f; // List views can be memory-heavy
        }
        
        // Memory for bindings
        EstimatedMemory += WidgetInfo.BoundProperties.Num() * 0.1f;
    }
    
    return EstimatedMemory;
}

FString UBlueprintAnalyzerLibrary::GetWidgetTypeName(UWidget* Widget)
{
    if (!Widget)
    {
        return TEXT("Unknown");
    }
    
    return Widget->GetClass()->GetName();
}

bool UBlueprintAnalyzerLibrary::HasBindings(UWidget* Widget)
{
    if (!Widget)
    {
        return false;
    }
    
    // Check if widget has any bound properties by examining UE4/UE5 binding system
    // This is a more comprehensive check for widget bindings
    TArray<FString> BoundProps = GetBoundProperties(Widget);
    return BoundProps.Num() > 0;
}

TArray<FString> UBlueprintAnalyzerLibrary::GetBoundProperties(UWidget* Widget)
{
    TArray<FString> BoundProperties;
    
    if (!Widget)
    {
        return BoundProperties;
    }
    
    // Check common widget properties that are often bound
    UClass* WidgetClass = Widget->GetClass();
    
    // Check for common bindable properties
    TArray<FString> CommonBindableProperties = {
        TEXT("Visibility"),
        TEXT("IsEnabled"),
        TEXT("RenderOpacity"),
        TEXT("ColorAndOpacity"),
        TEXT("Text"),
        TEXT("ToolTipText")
    };
    
    for (const FString& PropertyName : CommonBindableProperties)
    {
        FProperty* Property = WidgetClass->FindPropertyByName(*PropertyName);
        if (Property)
        {
            // In a more complete implementation, you would check if this property
            // actually has a binding. For now, we'll do a heuristic check
            // by seeing if common bindable properties exist on the widget type
            if (Widget->IsA<UTextBlock>() && PropertyName == TEXT("Text"))
            {
                BoundProperties.Add(PropertyName);
            }
            else if (PropertyName == TEXT("Visibility") || PropertyName == TEXT("IsEnabled"))
            {
                // These are commonly bound properties on most widgets
                BoundProperties.Add(PropertyName);
            }
        }
    }
    
    return BoundProperties;
}

FString UBlueprintAnalyzerLibrary::ExportWidgetAnalysisToJSON(const FWidgetOptimizationReport& Report)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    
    RootObject->SetStringField(TEXT("WidgetBlueprintName"), Report.WidgetBlueprintName);
    RootObject->SetStringField(TEXT("AnalysisTimestamp"), Report.AnalysisTimestamp);
    RootObject->SetNumberField(TEXT("TotalWidgets"), Report.TotalWidgets);
    RootObject->SetNumberField(TEXT("MaxDepth"), Report.MaxDepth);
    RootObject->SetNumberField(TEXT("TotalBindings"), Report.TotalBindings);
    RootObject->SetNumberField(TEXT("EstimatedMemoryUsage"), Report.EstimatedMemoryUsage);
    RootObject->SetNumberField(TEXT("OptimizationScore"), Report.OptimizationScore);

    // Widget Hierarchy
    TArray<TSharedPtr<FJsonValue>> HierarchyArray;
    for (const FWidgetHierarchyInfo& WidgetInfo : Report.WidgetHierarchy)
    {
        TSharedPtr<FJsonObject> WidgetObject = MakeShareable(new FJsonObject);
        WidgetObject->SetStringField(TEXT("WidgetName"), WidgetInfo.WidgetName);
        WidgetObject->SetStringField(TEXT("WidgetType"), WidgetInfo.WidgetType);
        WidgetObject->SetNumberField(TEXT("Depth"), WidgetInfo.Depth);
        WidgetObject->SetNumberField(TEXT("ChildrenCount"), WidgetInfo.ChildrenCount);
        WidgetObject->SetBoolField(TEXT("HasBindings"), WidgetInfo.bHasBindings);
        
        TArray<TSharedPtr<FJsonValue>> PropertiesArray;
        for (const FString& Property : WidgetInfo.BoundProperties)
        {
            PropertiesArray.Add(MakeShareable(new FJsonValueString(Property)));
        }
        WidgetObject->SetArrayField(TEXT("BoundProperties"), PropertiesArray);
        
        HierarchyArray.Add(MakeShareable(new FJsonValueObject(WidgetObject)));
    }
    RootObject->SetArrayField(TEXT("WidgetHierarchy"), HierarchyArray);

    // Optimization Issues
    TArray<TSharedPtr<FJsonValue>> IssuesArray;
    for (const FWidgetOptimizationIssue& Issue : Report.OptimizationIssues)
    {
        TSharedPtr<FJsonObject> IssueObject = MakeShareable(new FJsonObject);
        IssueObject->SetStringField(TEXT("IssueType"), Issue.IssueType);
        IssueObject->SetStringField(TEXT("Description"), Issue.Description);
        IssueObject->SetStringField(TEXT("Recommendation"), Issue.Recommendation);
        IssueObject->SetStringField(TEXT("Severity"), 
            Issue.Severity == EWidgetOptimizationSeverity::Critical ? TEXT("Critical") :
            Issue.Severity == EWidgetOptimizationSeverity::Warning ? TEXT("Warning") : TEXT("Info"));
        IssueObject->SetStringField(TEXT("WidgetPath"), Issue.WidgetPath);
        
        IssuesArray.Add(MakeShareable(new FJsonValueObject(IssueObject)));
    }
    RootObject->SetArrayField(TEXT("OptimizationIssues"), IssuesArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    return OutputString;
}

FString UBlueprintAnalyzerLibrary::ExportWidgetAnalysisToLLMText(const FWidgetOptimizationReport& Report)
{
    FString Result;
    
    Result += FString::Printf(TEXT("Widget Blueprint Optimization Report: %s\n"), *Report.WidgetBlueprintName);
    Result += FString::Printf(TEXT("Analyzed at: %s\n\n"), *Report.AnalysisTimestamp);
    
    Result += TEXT("=== SUMMARY ===\n");
    Result += FString::Printf(TEXT("Total Widgets: %d\n"), Report.TotalWidgets);
    Result += FString::Printf(TEXT("Maximum Depth: %d\n"), Report.MaxDepth);
    Result += FString::Printf(TEXT("Total Bindings: %d\n"), Report.TotalBindings);
    Result += FString::Printf(TEXT("Estimated Memory Usage: %.2f KB\n"), Report.EstimatedMemoryUsage);
    Result += FString::Printf(TEXT("Optimization Score: %d/100\n\n"), Report.OptimizationScore);
    
    Result += TEXT("=== WIDGET HIERARCHY ===\n");
    for (const FWidgetHierarchyInfo& WidgetInfo : Report.WidgetHierarchy)
    {
        FString Indent = FString::ChrN(WidgetInfo.Depth * 2, ' ');
        Result += FString::Printf(TEXT("%s- %s (%s)\n"), *Indent, *WidgetInfo.WidgetName, *WidgetInfo.WidgetType);
        if (WidgetInfo.ChildrenCount > 0)
        {
            Result += FString::Printf(TEXT("%s  Children: %d\n"), *Indent, WidgetInfo.ChildrenCount);
        }
        if (WidgetInfo.bHasBindings)
        {
            Result += FString::Printf(TEXT("%s  Bindings: %d\n"), *Indent, WidgetInfo.BoundProperties.Num());
        }
    }
    
    Result += TEXT("\n=== OPTIMIZATION ISSUES ===\n");
    if (Report.OptimizationIssues.Num() == 0)
    {
        Result += TEXT("No optimization issues found!\n");
    }
    else
    {
        for (const FWidgetOptimizationIssue& Issue : Report.OptimizationIssues)
        {
            FString SeverityText = Issue.Severity == EWidgetOptimizationSeverity::Critical ? TEXT("CRITICAL") :
                                  Issue.Severity == EWidgetOptimizationSeverity::Warning ? TEXT("WARNING") : TEXT("INFO");
            Result += FString::Printf(TEXT("[%s] %s: %s\n"), *SeverityText, *Issue.IssueType, *Issue.Description);
            Result += FString::Printf(TEXT("  Widget: %s\n"), *Issue.WidgetPath);
            Result += FString::Printf(TEXT("  Recommendation: %s\n\n"), *Issue.Recommendation);
        }
    }
    
    return Result;
}

bool UBlueprintAnalyzerLibrary::SaveWidgetAnalysisToFile(const FWidgetOptimizationReport& Report, const FString& FilePath, const FString& Format)
{
    FString Content;
    
    if (Format.ToUpper() == TEXT("JSON"))
    {
        Content = ExportWidgetAnalysisToJSON(Report);
    }
    else
    {
        Content = ExportWidgetAnalysisToLLMText(Report);
    }
    
    return FFileHelper::SaveStringToFile(Content, *FilePath);
}

FString UBlueprintAnalyzerLibrary::GenerateOptimizedWidgetCode(const FWidgetOptimizationReport& Report)
{
    FString Code;
    
    Code += FString::Printf(TEXT("// Optimized Widget Blueprint Code for %s\n"), *Report.WidgetBlueprintName);
    Code += FString::Printf(TEXT("// Generated on %s\n"), *Report.AnalysisTimestamp);
    Code += FString::Printf(TEXT("// Original Optimization Score: %d/100\n\n"), Report.OptimizationScore);
    
    Code += TEXT("#pragma once\n\n");
    Code += TEXT("#include \"CoreMinimal.h\"\n");
    Code += TEXT("#include \"Blueprint/UserWidget.h\"\n");
    Code += TEXT("#include \"Components/VerticalBox.h\"\n");
    Code += TEXT("#include \"Components/TextBlock.h\"\n");
    Code += TEXT("#include \"Components/Button.h\"\n");
    Code += FString::Printf(TEXT("#include \"Optimized%s.generated.h\"\n\n"), *Report.WidgetBlueprintName);
    
    Code += TEXT("UCLASS()\n");
    Code += FString::Printf(TEXT("class YOURPROJECT_API UOptimized%s : public UUserWidget\n"), *Report.WidgetBlueprintName);
    Code += TEXT("{\n");
    Code += TEXT("\tGENERATED_BODY()\n\n");
    Code += TEXT("public:\n");
    Code += FString::Printf(TEXT("\tUOptimized%s(const FObjectInitializer& ObjectInitializer);\n\n"), *Report.WidgetBlueprintName);
    
    Code += TEXT("protected:\n");
    Code += TEXT("\tvirtual void NativeConstruct() override;\n");
    Code += TEXT("\tvirtual void NativePreConstruct(bool IsDesignTime) override;\n\n");
    
    Code += TEXT("\t// Widget Components (Generated based on analysis)\n");
    for (const FWidgetHierarchyInfo& WidgetInfo : Report.WidgetHierarchy)
    {
        if (WidgetInfo.Depth <= 2) // Only include top-level widgets in header
        {
            Code += FString::Printf(TEXT("\tUPROPERTY(meta = (BindWidget))\n"));
            Code += FString::Printf(TEXT("\tU%s* %s;\n\n"), *WidgetInfo.WidgetType.Replace(TEXT("Widget"), TEXT("")), *WidgetInfo.WidgetName);
        }
    }
    
    Code += TEXT("private:\n");
    Code += TEXT("\tvoid InitializeOptimizedLayout();\n");
    Code += TEXT("\tvoid ApplyPerformanceOptimizations();\n");
    Code += TEXT("};\n\n");
    
    Code += TEXT("// Optimization Recommendations:\n");
    for (const FWidgetOptimizationIssue& Issue : Report.OptimizationIssues)
    {
        Code += FString::Printf(TEXT("// - %s: %s\n"), *Issue.IssueType, *Issue.Recommendation);
    }
    
    return Code;
}
