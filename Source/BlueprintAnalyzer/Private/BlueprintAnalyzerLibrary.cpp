// Copyright (c) 2025 keemminxu. All Rights Reserved.

#include "BlueprintAnalyzerLibrary.h"
#include "K2Node.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Timeline.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_Knot.h"
#include "K2Node_Composite.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphNode_Comment.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/TimelineTemplate.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/UObjectIterator.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"
#include "Animation/AnimInstance.h"
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
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

// Forward declarations for file-local static helpers (used across sections)
static TArray<TSharedPtr<FJsonValue>> StringArrayToJson(const TArray<FString>& InArray);
static TArray<TSharedPtr<FJsonValue>> ParamsToJson(const TArray<FBPFunctionParam>& Params);
static TSharedPtr<FJsonObject> MetadataToJson(const FBPAnalyzerMetadata& Meta);

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
    Result.Metadata = ExtractMetadata(Blueprint);

    auto ProcessGraphs = [&Result](const TArray<UEdGraph*>& Graphs)
    {
        for (UEdGraph* Graph : Graphs)
        {
            if (!Graph) continue;

            const FString GraphName = Graph->GetName();
            for (UEdGraphNode* GraphNode : Graph->Nodes)
            {
                if (UK2Node* K2Node = Cast<UK2Node>(GraphNode))
                {
                    FBlueprintNodeInfo NodeInfo = ExtractNodeInfo(K2Node);
                    NodeInfo.GraphName = GraphName;
                    NodeInfo.CommentGroup = FindCommentGroupForNode(K2Node, Graph);
                    Result.Nodes.Add(NodeInfo);
                }
            }

            Result.Connections.Append(ExtractConnections(Graph));
        }
    };

    ProcessGraphs(Blueprint->UbergraphPages);
    ProcessGraphs(Blueprint->FunctionGraphs);
    ProcessGraphs(Blueprint->MacroGraphs);
    ProcessGraphs(Blueprint->DelegateSignatureGraphs);
    ProcessGraphs(Blueprint->IntermediateGeneratedGraphs);

    Result.ExecutionPaths = TraceExecutionPaths(Blueprint);

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
    else if (UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node))
    {
        if (CastNode->TargetType)
        {
            NodeInfo.FunctionName = CastNode->TargetType->GetName();
        }
    }
    else if (UK2Node_MacroInstance* MacroNode = Cast<UK2Node_MacroInstance>(Node))
    {
        if (UEdGraph* MacroGraph = MacroNode->GetMacroGraph())
        {
            NodeInfo.FunctionName = MacroGraph->GetName();
        }
    }

    for (UEdGraphPin* Pin : Node->Pins)
    {
        const FString PinEntry = FString::Printf(TEXT("%s:%s"),
            *Pin->PinName.ToString(),
            *GetPinTypeString(Pin->PinType));

        if (Pin->Direction == EGPD_Input)
        {
            NodeInfo.InputPins.Add(PinEntry);

            const FString Literal = ExtractLiteralFromPin(Pin);
            if (!Literal.IsEmpty())
            {
                NodeInfo.LiteralValues.Add(FString::Printf(TEXT("%s=%s"), *Pin->PinName.ToString(), *Literal));
            }
        }
        else if (Pin->Direction == EGPD_Output)
        {
            NodeInfo.OutputPins.Add(PinEntry);
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

    if (Cast<UK2Node_CustomEvent>(Node)) return TEXT("CustomEvent");
    if (Cast<UK2Node_Event>(Node)) return TEXT("Event");
    if (Cast<UK2Node_FunctionEntry>(Node)) return TEXT("FunctionEntry");
    if (Cast<UK2Node_FunctionResult>(Node)) return TEXT("FunctionResult");
    if (Cast<UK2Node_CallFunction>(Node)) return TEXT("FunctionCall");
    if (Cast<UK2Node_VariableGet>(Node)) return TEXT("VariableGet");
    if (Cast<UK2Node_VariableSet>(Node)) return TEXT("VariableSet");
    if (Cast<UK2Node_IfThenElse>(Node)) return TEXT("Branch");
    if (Cast<UK2Node_ExecutionSequence>(Node)) return TEXT("Sequence");
    if (Cast<UK2Node_DynamicCast>(Node)) return TEXT("Cast");
    if (Cast<UK2Node_SpawnActorFromClass>(Node)) return TEXT("SpawnActor");
    if (Cast<UK2Node_MacroInstance>(Node)) return TEXT("Macro");
    if (Cast<UK2Node_Timeline>(Node)) return TEXT("Timeline");
    if (Cast<UK2Node_Composite>(Node)) return TEXT("CollapsedGraph");
    if (Cast<UK2Node_Knot>(Node)) return TEXT("Reroute");

    return Node->GetClass()->GetName();
}

// ============================================================
// Phase 1: Metadata Extraction
// ============================================================

FString UBlueprintAnalyzerLibrary::GetPinTypeString(const FEdGraphPinType& PinType)
{
    FString TypeString = PinType.PinCategory.ToString();

    if (UObject* SubObject = PinType.PinSubCategoryObject.Get())
    {
        TypeString += TEXT("<") + SubObject->GetName() + TEXT(">");
    }
    else if (!PinType.PinSubCategory.IsNone())
    {
        TypeString += TEXT("<") + PinType.PinSubCategory.ToString() + TEXT(">");
    }

    if (PinType.IsArray())
    {
        TypeString = TEXT("Array<") + TypeString + TEXT(">");
    }
    else if (PinType.IsSet())
    {
        TypeString = TEXT("Set<") + TypeString + TEXT(">");
    }
    else if (PinType.IsMap())
    {
        const FString ValueType = PinType.PinValueType.TerminalSubCategoryObject.IsValid()
            ? PinType.PinValueType.TerminalSubCategoryObject->GetName()
            : PinType.PinValueType.TerminalCategory.ToString();
        TypeString = FString::Printf(TEXT("Map<%s,%s>"), *TypeString, *ValueType);
    }

    if (PinType.bIsReference) TypeString += TEXT("&");
    if (PinType.bIsConst) TypeString = TEXT("const ") + TypeString;

    return TypeString;
}

FString UBlueprintAnalyzerLibrary::GetPropertyTypeString(const FProperty* Property)
{
    if (!Property) return TEXT("Unknown");
    return Property->GetCPPType();
}

FString UBlueprintAnalyzerLibrary::ExtractLiteralFromPin(const UEdGraphPin* Pin)
{
    if (!Pin || Pin->Direction != EGPD_Input) return FString();
    if (Pin->LinkedTo.Num() > 0) return FString();
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec) return FString();

    if (!Pin->DefaultValue.IsEmpty())
    {
        return Pin->DefaultValue;
    }
    if (Pin->DefaultObject)
    {
        return Pin->DefaultObject->GetName();
    }
    if (!Pin->DefaultTextValue.IsEmpty())
    {
        return Pin->DefaultTextValue.ToString();
    }
    return FString();
}

FString UBlueprintAnalyzerLibrary::FindCommentGroupForNode(const UEdGraphNode* Node, const UEdGraph* Graph)
{
    if (!Node || !Graph) return FString();

    const float NodeLeft = Node->NodePosX;
    const float NodeTop = Node->NodePosY;
    const float NodeRight = NodeLeft + FMath::Max(Node->NodeWidth, 100);
    const float NodeBottom = NodeTop + FMath::Max(Node->NodeHeight, 50);

    for (UEdGraphNode* OtherNode : Graph->Nodes)
    {
        const UEdGraphNode_Comment* Comment = Cast<UEdGraphNode_Comment>(OtherNode);
        if (!Comment) continue;

        const float CL = Comment->NodePosX;
        const float CT = Comment->NodePosY;
        const float CR = CL + FMath::Max(Comment->NodeWidth, 0);
        const float CB = CT + FMath::Max(Comment->NodeHeight, 0);

        if (NodeLeft >= CL && NodeRight <= CR && NodeTop >= CT && NodeBottom <= CB)
        {
            return Comment->NodeComment;
        }
    }

    return FString();
}

FString UBlueprintAnalyzerLibrary::GetBlueprintTypeString(UBlueprint* Blueprint)
{
    if (!Blueprint) return TEXT("Unknown");

    if (Cast<UWidgetBlueprint>(Blueprint)) return TEXT("WidgetBlueprint");

    if (UClass* GenClass = Blueprint->GeneratedClass)
    {
        if (GenClass->IsChildOf<UAnimInstance>()) return TEXT("AnimBlueprint");
        if (GenClass->IsChildOf<AActor>()) return TEXT("ActorBlueprint");
        if (GenClass->IsChildOf<UActorComponent>()) return TEXT("ComponentBlueprint");
        if (GenClass->HasAnyClassFlags(CLASS_Interface)) return TEXT("InterfaceBlueprint");
    }

    switch (Blueprint->BlueprintType)
    {
    case BPTYPE_Normal: return TEXT("Blueprint");
    case BPTYPE_Const: return TEXT("ConstBlueprint");
    case BPTYPE_MacroLibrary: return TEXT("MacroLibrary");
    case BPTYPE_Interface: return TEXT("Interface");
    case BPTYPE_LevelScript: return TEXT("LevelScript");
    case BPTYPE_FunctionLibrary: return TEXT("FunctionLibrary");
    default: return TEXT("Blueprint");
    }
}

FBPAnalyzerMetadata UBlueprintAnalyzerLibrary::ExtractMetadata(UBlueprint* Blueprint)
{
    FBPAnalyzerMetadata Metadata;
    if (!Blueprint) return Metadata;

    Metadata.BlueprintType = GetBlueprintTypeString(Blueprint);

    // Parent class
    if (Blueprint->ParentClass)
    {
        Metadata.ParentClass = Blueprint->ParentClass->GetName();
    }

    // Implemented interfaces
    for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
    {
        if (Interface.Interface)
        {
            Metadata.ImplementedInterfaces.Add(Interface.Interface->GetName());
        }
    }

    // Variables
    for (const FBPVariableDescription& Var : Blueprint->NewVariables)
    {
        FBPVariableInfo Info;
        Info.VariableName = Var.VarName.ToString();
        Info.VariableType = GetPinTypeString(Var.VarType);
        Info.Category = Var.Category.ToString();
        Info.DefaultValue = Var.DefaultValue;
        Info.bEditable = (Var.PropertyFlags & CPF_Edit) != 0;
        Info.bReplicated = (Var.PropertyFlags & CPF_Net) != 0;
        Info.bExposeOnSpawn = (Var.PropertyFlags & CPF_ExposeOnSpawn) != 0;

        for (const FBPVariableMetaDataEntry& Meta : Var.MetaDataArray)
        {
            if (Meta.DataKey == FName(TEXT("tooltip")))
            {
                Info.Tooltip = Meta.DataValue;
                break;
            }
        }

        Metadata.Variables.Add(Info);
    }

    // Custom functions (walk FunctionGraphs, find FunctionEntry to read signature)
    for (UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
    {
        if (!FunctionGraph) continue;

        FBPFunctionSignature Signature;
        Signature.FunctionName = FunctionGraph->GetName();

        for (UEdGraphNode* GraphNode : FunctionGraph->Nodes)
        {
            if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(GraphNode))
            {
                Signature.bPure = (Entry->GetFunctionFlags() & FUNC_BlueprintPure) != 0;
                Signature.bStatic = (Entry->GetFunctionFlags() & FUNC_Static) != 0;
                Signature.bConst = (Entry->GetFunctionFlags() & FUNC_Const) != 0;

                const uint32 AccessFlags = Entry->GetFunctionFlags() & FUNC_AccessSpecifiers;
                if (AccessFlags & FUNC_Private) Signature.AccessSpecifier = TEXT("Private");
                else if (AccessFlags & FUNC_Protected) Signature.AccessSpecifier = TEXT("Protected");
                else Signature.AccessSpecifier = TEXT("Public");

                Signature.Category = Entry->MetaData.Category.ToString();

                for (UEdGraphPin* Pin : Entry->Pins)
                {
                    if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
                    {
                        FBPFunctionParam Param;
                        Param.ParamName = Pin->PinName.ToString();
                        Param.ParamType = GetPinTypeString(Pin->PinType);
                        Param.bIsReference = Pin->PinType.bIsReference;
                        Param.bIsConst = Pin->PinType.bIsConst;
                        Signature.Parameters.Add(Param);
                    }
                }
            }
            else if (UK2Node_FunctionResult* ReturnNode = Cast<UK2Node_FunctionResult>(GraphNode))
            {
                for (UEdGraphPin* Pin : ReturnNode->Pins)
                {
                    if (Pin->Direction == EGPD_Input && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
                    {
                        if (Signature.ReturnType.IsEmpty())
                        {
                            Signature.ReturnType = GetPinTypeString(Pin->PinType);
                        }
                        else
                        {
                            FBPFunctionParam Param;
                            Param.ParamName = Pin->PinName.ToString();
                            Param.ParamType = GetPinTypeString(Pin->PinType);
                            Param.bIsReturn = true;
                            Signature.Parameters.Add(Param);
                        }
                    }
                }
            }
        }

        Metadata.CustomFunctions.Add(Signature);
    }

    // Components (Actor BPs only)
    if (Blueprint->SimpleConstructionScript)
    {
        for (USCS_Node* SCSNode : Blueprint->SimpleConstructionScript->GetAllNodes())
        {
            if (!SCSNode) continue;
            FBPComponentInfo CompInfo;
            CompInfo.ComponentName = SCSNode->GetVariableName().ToString();
            if (SCSNode->ComponentClass)
            {
                CompInfo.ComponentType = SCSNode->ComponentClass->GetName();
            }
            CompInfo.AttachSocketName = SCSNode->AttachToName.ToString();

            if (USCS_Node* Parent = Blueprint->SimpleConstructionScript->FindParentNode(SCSNode))
            {
                CompInfo.ParentComponentName = Parent->GetVariableName().ToString();
            }

            Metadata.Components.Add(CompInfo);
        }
    }

    // Event Dispatchers (represented as DelegateSignatureGraphs)
    for (UEdGraph* DelegateGraph : Blueprint->DelegateSignatureGraphs)
    {
        if (!DelegateGraph) continue;

        FBPEventDispatcherInfo Info;
        Info.DispatcherName = DelegateGraph->GetName();

        for (UEdGraphNode* GraphNode : DelegateGraph->Nodes)
        {
            if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(GraphNode))
            {
                for (UEdGraphPin* Pin : Entry->Pins)
                {
                    if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
                    {
                        FBPFunctionParam Param;
                        Param.ParamName = Pin->PinName.ToString();
                        Param.ParamType = GetPinTypeString(Pin->PinType);
                        Info.Parameters.Add(Param);
                    }
                }
            }
        }

        Metadata.EventDispatchers.Add(Info);
    }

    // Macros (graph names only — full contents come through ProcessGraphs in AnalyzeBlueprint)
    for (UEdGraph* MacroGraph : Blueprint->MacroGraphs)
    {
        if (MacroGraph)
        {
            Metadata.MacroNames.Add(MacroGraph->GetName());
        }
    }

    // Timelines
    for (UTimelineTemplate* Timeline : Blueprint->Timelines)
    {
        if (Timeline)
        {
            Metadata.TimelineNames.Add(Timeline->GetName());
        }
    }

    return Metadata;
}

// ============================================================
// Phase 2: Execution Flow Tracing
// ============================================================

FString UBlueprintAnalyzerLibrary::GetExecutionStepSummary(UK2Node* Node)
{
    if (!Node) return TEXT("");

    const FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
    const FString TypeName = GetNodeTypeName(Node);

    if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node))
    {
        if (CallNode->GetTargetFunction())
        {
            return FString::Printf(TEXT("%s(...)"), *CallNode->GetTargetFunction()->GetName());
        }
    }
    if (UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node))
    {
        if (CastNode->TargetType)
        {
            return FString::Printf(TEXT("Cast<%s>"), *CastNode->TargetType->GetName());
        }
    }
    if (UK2Node_MacroInstance* MacroNode = Cast<UK2Node_MacroInstance>(Node))
    {
        if (UEdGraph* MacroGraph = MacroNode->GetMacroGraph())
        {
            return FString::Printf(TEXT("Macro: %s"), *MacroGraph->GetName());
        }
    }

    return FString::Printf(TEXT("%s (%s)"), *TypeName, *Title);
}

void UBlueprintAnalyzerLibrary::TraceFromNode(
    UK2Node* StartNode,
    int32 Depth,
    const FString& BranchLabel,
    TArray<FExecutionStep>& OutSteps,
    TSet<FGuid>& VisitedNodes)
{
    // Safety: depth cap and cycle guard
    const int32 MaxDepth = 64;
    const int32 MaxSteps = 512;
    if (!StartNode || Depth > MaxDepth || OutSteps.Num() > MaxSteps) return;

    UK2Node* Current = StartNode;
    int32 CurrentDepth = Depth;
    FString CurrentLabel = BranchLabel;

    while (Current)
    {
        if (OutSteps.Num() > MaxSteps) return;

        // Cycle detection — if we've already visited this node, add a marker and stop
        if (VisitedNodes.Contains(Current->NodeGuid))
        {
            FExecutionStep Step;
            Step.NodeGuid = Current->NodeGuid.ToString();
            Step.NodeType = TEXT("Loopback");
            Step.Summary = FString::Printf(TEXT("-> back to %s"), *GetExecutionStepSummary(Current));
            Step.BranchLabel = CurrentLabel;
            Step.Depth = CurrentDepth;
            Step.bIsTerminator = true;
            OutSteps.Add(Step);
            return;
        }
        VisitedNodes.Add(Current->NodeGuid);

        // Record this step
        FExecutionStep Step;
        Step.NodeGuid = Current->NodeGuid.ToString();
        Step.NodeType = GetNodeTypeName(Current);
        Step.Summary = GetExecutionStepSummary(Current);
        Step.BranchLabel = CurrentLabel;
        Step.Depth = CurrentDepth;

        // Latent node detection: CallFunction flagged with MD_Latent metadata
        if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Current))
        {
            if (UFunction* Fn = CallNode->GetTargetFunction())
            {
                Step.bIsLatent = Fn->HasMetaData(FBlueprintMetadata::MD_Latent);
            }
        }

        OutSteps.Add(Step);
        CurrentLabel.Reset();

        // Branch handling: Branch (IfThenElse), Sequence, Cast with success/fail
        if (UK2Node_IfThenElse* BranchNode = Cast<UK2Node_IfThenElse>(Current))
        {
            UEdGraphPin* ThenPin = BranchNode->GetThenPin();
            UEdGraphPin* ElsePin = BranchNode->GetElsePin();
            if (ThenPin && ThenPin->LinkedTo.Num() > 0)
            {
                for (UEdGraphPin* Linked : ThenPin->LinkedTo)
                {
                    if (UK2Node* NextNode = Cast<UK2Node>(Linked->GetOwningNode()))
                    {
                        TraceFromNode(NextNode, CurrentDepth + 1, TEXT("True"), OutSteps, VisitedNodes);
                    }
                }
            }
            if (ElsePin && ElsePin->LinkedTo.Num() > 0)
            {
                for (UEdGraphPin* Linked : ElsePin->LinkedTo)
                {
                    if (UK2Node* NextNode = Cast<UK2Node>(Linked->GetOwningNode()))
                    {
                        TraceFromNode(NextNode, CurrentDepth + 1, TEXT("False"), OutSteps, VisitedNodes);
                    }
                }
            }
            return;
        }

        if (UK2Node_ExecutionSequence* SequenceNode = Cast<UK2Node_ExecutionSequence>(Current))
        {
            int32 BranchIdx = 0;
            for (UEdGraphPin* Pin : SequenceNode->Pins)
            {
                if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
                {
                    for (UEdGraphPin* Linked : Pin->LinkedTo)
                    {
                        if (UK2Node* NextNode = Cast<UK2Node>(Linked->GetOwningNode()))
                        {
                            TraceFromNode(NextNode, CurrentDepth + 1, FString::Printf(TEXT("Then %d"), BranchIdx), OutSteps, VisitedNodes);
                        }
                    }
                    BranchIdx++;
                }
            }
            return;
        }

        // Default: follow the single "then" exec output
        UK2Node* NextInChain = nullptr;
        for (UEdGraphPin* Pin : Current->Pins)
        {
            if (Pin->Direction == EGPD_Output &&
                Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec &&
                Pin->LinkedTo.Num() > 0)
            {
                // Use the first linked node as the next in chain
                if (UEdGraphPin* Linked = Pin->LinkedTo[0])
                {
                    NextInChain = Cast<UK2Node>(Linked->GetOwningNode());

                    // If this node has multiple exec outputs (e.g., SpawnActor success/fail, Cast), branch them
                    if (Pin->LinkedTo.Num() > 1)
                    {
                        for (int32 i = 1; i < Pin->LinkedTo.Num(); ++i)
                        {
                            if (UEdGraphPin* OtherLinked = Pin->LinkedTo[i])
                            {
                                if (UK2Node* OtherNode = Cast<UK2Node>(OtherLinked->GetOwningNode()))
                                {
                                    TraceFromNode(OtherNode, CurrentDepth + 1, Pin->PinName.ToString(), OutSteps, VisitedNodes);
                                }
                            }
                        }
                    }
                }
                break;
            }
        }

        Current = NextInChain;
    }
}

// ============================================================
// Phase 3: Blueprint Performance Analysis
// ============================================================

// Helper: identify Tick / BeginPlay entry points
static bool IsTickEvent(UK2Node* Node)
{
    if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
    {
        const FString Name = EventNode->EventReference.GetMemberName().ToString();
        return Name.Equals(TEXT("ReceiveTick"), ESearchCase::IgnoreCase) ||
               Name.Equals(TEXT("Tick"), ESearchCase::IgnoreCase);
    }
    return false;
}

static bool IsBeginPlayEvent(UK2Node* Node)
{
    if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
    {
        const FString Name = EventNode->EventReference.GetMemberName().ToString();
        return Name.Equals(TEXT("ReceiveBeginPlay"), ESearchCase::IgnoreCase) ||
               Name.Equals(TEXT("BeginPlay"), ESearchCase::IgnoreCase);
    }
    return false;
}

// Walks exec-flow downstream from an event node and collects every reachable K2Node
static void CollectReachableNodes(UK2Node* Start, TSet<UK2Node*>& OutVisited)
{
    if (!Start || OutVisited.Contains(Start)) return;

    TArray<UK2Node*> Stack;
    Stack.Push(Start);

    while (Stack.Num() > 0)
    {
        UK2Node* Current = Stack.Pop();
        if (!Current || OutVisited.Contains(Current)) continue;
        OutVisited.Add(Current);

        for (UEdGraphPin* Pin : Current->Pins)
        {
            if (Pin->Direction == EGPD_Output &&
                Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
            {
                for (UEdGraphPin* Linked : Pin->LinkedTo)
                {
                    if (UK2Node* NextNode = Cast<UK2Node>(Linked->GetOwningNode()))
                    {
                        Stack.Push(NextNode);
                    }
                }
            }
        }
    }
}

// Detect expensive function calls that are known performance anti-patterns
static bool IsExpensiveFunctionCall(UK2Node_CallFunction* CallNode, FString& OutFunctionName)
{
    if (!CallNode || !CallNode->GetTargetFunction()) return false;

    const FString FuncName = CallNode->GetTargetFunction()->GetName();
    OutFunctionName = FuncName;

    static const TArray<FString> ExpensiveFunctions = {
        TEXT("GetAllActorsOfClass"),
        TEXT("GetAllActorsWithInterface"),
        TEXT("GetAllActorsWithTag"),
        TEXT("GetAllWidgetsOfClass"),
        TEXT("LineTraceSingle"),
        TEXT("LineTraceMulti"),
        TEXT("SphereTraceSingle"),
        TEXT("SphereTraceMulti"),
    };

    for (const FString& Expensive : ExpensiveFunctions)
    {
        if (FuncName.Contains(Expensive)) return true;
    }

    return false;
}

FBPPerformanceReport UBlueprintAnalyzerLibrary::AnalyzeBlueprintPerformance(UBlueprint* Blueprint)
{
    FBPPerformanceReport Report;
    if (!Blueprint) return Report;

    Report.BlueprintName = Blueprint->GetName();
    Report.AnalysisTimestamp = FDateTime::Now().ToString();

    // Collect all event graph nodes
    TArray<UK2Node*> AllNodes;
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (!Graph) continue;
        for (UEdGraphNode* GraphNode : Graph->Nodes)
        {
            if (UK2Node* K2Node = Cast<UK2Node>(GraphNode))
            {
                AllNodes.Add(K2Node);
            }
        }
    }
    Report.TotalNodes = AllNodes.Num();

    // Gather nodes reachable from Tick/BeginPlay
    TSet<UK2Node*> TickNodes;
    TSet<UK2Node*> BeginPlayNodes;
    for (UK2Node* Node : AllNodes)
    {
        if (IsTickEvent(Node))
        {
            CollectReachableNodes(Node, TickNodes);
        }
        else if (IsBeginPlayEvent(Node))
        {
            CollectReachableNodes(Node, BeginPlayNodes);
        }
        if (Node->IsA<UK2Node_Event>() || Node->IsA<UK2Node_CustomEvent>())
        {
            Report.EventCount++;
        }
        if (Node->IsA<UK2Node_DynamicCast>())
        {
            Report.CastCount++;
        }
    }
    Report.TickNodeCount = TickNodes.Num();
    Report.BeginPlayNodeCount = BeginPlayNodes.Num();

    // Rule 1: expensive calls inside Tick (-25 each, cap at 3)
    int32 TickExpensiveCount = 0;
    for (UK2Node* Node : TickNodes)
    {
        if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node))
        {
            FString FuncName;
            if (IsExpensiveFunctionCall(CallNode, FuncName))
            {
                FBPPerformanceIssue Issue;
                Issue.IssueType = TEXT("Expensive Call in Tick");
                Issue.Description = FString::Printf(TEXT("'%s' is called every frame inside Tick"), *FuncName);
                Issue.Recommendation = TEXT("Cache the result in BeginPlay, use a timer, or event-driven alternative (-25 points)");
                Issue.Severity = EBPPerformanceSeverity::Critical;
                Issue.NodeGuid = Node->NodeGuid.ToString();
                Issue.Deduction = 25;
                if (UEdGraph* G = Node->GetGraph()) Issue.GraphName = G->GetName();
                Report.Issues.Add(Issue);
                if (++TickExpensiveCount >= 3) break;
            }
        }
    }

    // Rule 2: Cast node inside Tick (-10 each, cap at 3)
    int32 TickCastCount = 0;
    for (UK2Node* Node : TickNodes)
    {
        if (Node->IsA<UK2Node_DynamicCast>())
        {
            FBPPerformanceIssue Issue;
            Issue.IssueType = TEXT("Cast in Tick");
            Issue.Description = TEXT("Cast is performed every frame");
            Issue.Recommendation = TEXT("Cache the cast result in BeginPlay and reuse the pointer (-10 points)");
            Issue.Severity = EBPPerformanceSeverity::Warning;
            Issue.NodeGuid = Node->NodeGuid.ToString();
            Issue.Deduction = 10;
            if (UEdGraph* G = Node->GetGraph()) Issue.GraphName = G->GetName();
            Report.Issues.Add(Issue);
            if (++TickCastCount >= 3) break;
        }
    }

    // Rule 3: Tick graph size (-15 if > 50 nodes downstream)
    if (Report.TickNodeCount > 50)
    {
        FBPPerformanceIssue Issue;
        Issue.IssueType = TEXT("Heavy Tick Logic");
        Issue.Description = FString::Printf(TEXT("Tick event drives %d downstream nodes"), Report.TickNodeCount);
        Issue.Recommendation = TEXT("Break Tick work across frames, move to timers, or switch to event-driven design (-15 points)");
        Issue.Severity = EBPPerformanceSeverity::Warning;
        Issue.Deduction = 15;
        Report.Issues.Add(Issue);
    }

    // Rule 4: BeginPlay complexity (-10 if > 100 nodes downstream)
    if (Report.BeginPlayNodeCount > 100)
    {
        FBPPerformanceIssue Issue;
        Issue.IssueType = TEXT("Bloated BeginPlay");
        Issue.Description = FString::Printf(TEXT("BeginPlay drives %d downstream nodes"), Report.BeginPlayNodeCount);
        Issue.Recommendation = TEXT("Split initialization into smaller functions or defer heavy work (-10 points)");
        Issue.Severity = EBPPerformanceSeverity::Warning;
        Issue.Deduction = 10;
        Report.Issues.Add(Issue);
    }

    // Rule 5: excessive total casts (-5 if > 20)
    if (Report.CastCount > 20)
    {
        FBPPerformanceIssue Issue;
        Issue.IssueType = TEXT("Excessive Casts");
        Issue.Description = FString::Printf(TEXT("Blueprint contains %d Cast nodes"), Report.CastCount);
        Issue.Recommendation = TEXT("Use interfaces or cached references instead of repeated Casts (-5 points)");
        Issue.Severity = EBPPerformanceSeverity::Info;
        Issue.Deduction = 5;
        Report.Issues.Add(Issue);
    }

    // Final score
    int32 Score = 100;
    for (const FBPPerformanceIssue& Issue : Report.Issues)
    {
        Score -= Issue.Deduction;
    }
    Report.PerformanceScore = FMath::Clamp(Score, 0, 100);

    return Report;
}

FString UBlueprintAnalyzerLibrary::ExportPerformanceReportToJSON(const FBPPerformanceReport& Report)
{
    TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
    Root->SetStringField(TEXT("BlueprintName"), Report.BlueprintName);
    Root->SetStringField(TEXT("AnalysisTimestamp"), Report.AnalysisTimestamp);
    Root->SetNumberField(TEXT("TotalNodes"), Report.TotalNodes);
    Root->SetNumberField(TEXT("EventCount"), Report.EventCount);
    Root->SetNumberField(TEXT("CastCount"), Report.CastCount);
    Root->SetNumberField(TEXT("TickNodeCount"), Report.TickNodeCount);
    Root->SetNumberField(TEXT("BeginPlayNodeCount"), Report.BeginPlayNodeCount);
    Root->SetNumberField(TEXT("PerformanceScore"), Report.PerformanceScore);

    TArray<TSharedPtr<FJsonValue>> IssuesArr;
    for (const FBPPerformanceIssue& Issue : Report.Issues)
    {
        TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject);
        O->SetStringField(TEXT("IssueType"), Issue.IssueType);
        O->SetStringField(TEXT("Description"), Issue.Description);
        O->SetStringField(TEXT("Recommendation"), Issue.Recommendation);
        O->SetStringField(TEXT("Severity"),
            Issue.Severity == EBPPerformanceSeverity::Critical ? TEXT("Critical") :
            Issue.Severity == EBPPerformanceSeverity::Warning ? TEXT("Warning") : TEXT("Info"));
        O->SetStringField(TEXT("NodeGuid"), Issue.NodeGuid);
        O->SetStringField(TEXT("GraphName"), Issue.GraphName);
        O->SetNumberField(TEXT("Deduction"), Issue.Deduction);
        IssuesArr.Add(MakeShareable(new FJsonValueObject(O)));
    }
    Root->SetArrayField(TEXT("Issues"), IssuesArr);

    FString Out;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    return Out;
}

FString UBlueprintAnalyzerLibrary::ExportPerformanceReportToLLMText(const FBPPerformanceReport& Report)
{
    FString Out;
    Out += FString::Printf(TEXT("Blueprint Performance Report: %s\n"), *Report.BlueprintName);
    Out += FString::Printf(TEXT("Analyzed at: %s\n\n"), *Report.AnalysisTimestamp);
    Out += TEXT("=== SUMMARY ===\n");
    Out += FString::Printf(TEXT("Total Nodes: %d\n"), Report.TotalNodes);
    Out += FString::Printf(TEXT("Events: %d\n"), Report.EventCount);
    Out += FString::Printf(TEXT("Casts: %d\n"), Report.CastCount);
    Out += FString::Printf(TEXT("Tick downstream nodes: %d\n"), Report.TickNodeCount);
    Out += FString::Printf(TEXT("BeginPlay downstream nodes: %d\n"), Report.BeginPlayNodeCount);
    Out += FString::Printf(TEXT("Performance Score: %d/100\n\n"), Report.PerformanceScore);

    Out += TEXT("=== ISSUES ===\n");
    if (Report.Issues.Num() == 0)
    {
        Out += TEXT("No performance issues found.\n");
    }
    else
    {
        for (const FBPPerformanceIssue& Issue : Report.Issues)
        {
            const FString SeverityText = Issue.Severity == EBPPerformanceSeverity::Critical ? TEXT("CRITICAL") :
                                         Issue.Severity == EBPPerformanceSeverity::Warning ? TEXT("WARNING") : TEXT("INFO");
            Out += FString::Printf(TEXT("[%s] %s: %s\n"), *SeverityText, *Issue.IssueType, *Issue.Description);
            Out += FString::Printf(TEXT("  Recommendation: %s\n"), *Issue.Recommendation);
            if (!Issue.GraphName.IsEmpty())
            {
                Out += FString::Printf(TEXT("  Graph: %s\n"), *Issue.GraphName);
            }
            Out += TEXT("\n");
        }
    }

    return Out;
}

TArray<FExecutionPath> UBlueprintAnalyzerLibrary::TraceExecutionPaths(UBlueprint* Blueprint)
{
    TArray<FExecutionPath> Paths;
    if (!Blueprint) return Paths;

    auto ProcessGraphsForEntryPoints = [&Paths](const TArray<UEdGraph*>& Graphs)
    {
        for (UEdGraph* Graph : Graphs)
        {
            if (!Graph) continue;

            for (UEdGraphNode* GraphNode : Graph->Nodes)
            {
                UK2Node* K2Node = Cast<UK2Node>(GraphNode);
                if (!K2Node) continue;

                // Entry points: Event, CustomEvent, FunctionEntry
                const bool bIsEntry = K2Node->IsA<UK2Node_Event>() ||
                                      K2Node->IsA<UK2Node_CustomEvent>() ||
                                      K2Node->IsA<UK2Node_FunctionEntry>();
                if (!bIsEntry) continue;

                FExecutionPath Path;
                Path.EntryPointName = K2Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
                Path.EntryNodeGuid = K2Node->NodeGuid.ToString();
                Path.GraphName = Graph->GetName();

                TSet<FGuid> Visited;
                TraceFromNode(K2Node, 0, TEXT(""), Path.Steps, Visited);

                if (Path.Steps.Num() > 0)
                {
                    Paths.Add(Path);
                }
            }
        }
    };

    ProcessGraphsForEntryPoints(Blueprint->UbergraphPages);
    ProcessGraphsForEntryPoints(Blueprint->FunctionGraphs);

    return Paths;
}

// ============================================================
// Phase 4: Project Dependency Analysis + Batch
// ============================================================

int32 UBlueprintAnalyzerLibrary::EstimateTokenCount(const FString& Text)
{
    return FMath::CeilToInt(Text.Len() / 3.5f);
}

TArray<FBPDependency> UBlueprintAnalyzerLibrary::ExtractBlueprintDependencies(UBlueprint* Blueprint)
{
    TArray<FBPDependency> Out;
    if (!Blueprint) return Out;

    const FString SelfName = Blueprint->GetName();

    auto ProcessGraph = [&](UEdGraph* Graph)
    {
        if (!Graph) return;
        const FString GraphName = Graph->GetName();
        for (UEdGraphNode* GraphNode : Graph->Nodes)
        {
            UK2Node* K2Node = Cast<UK2Node>(GraphNode);
            if (!K2Node) continue;

            if (UK2Node_SpawnActorFromClass* SpawnNode = Cast<UK2Node_SpawnActorFromClass>(K2Node))
            {
                for (UEdGraphPin* Pin : SpawnNode->Pins)
                {
                    if (Pin->PinName == TEXT("Class") && Pin->DefaultObject)
                    {
                        FBPDependency Dep;
                        Dep.ReferencingBlueprint = SelfName;
                        Dep.ReferencedClass = Pin->DefaultObject->GetName();
                        Dep.ReferenceType = TEXT("Spawn");
                        Dep.NodeGuid = K2Node->NodeGuid.ToString();
                        Dep.GraphName = GraphName;
                        Out.Add(Dep);
                    }
                }
            }

            if (UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(K2Node))
            {
                if (CastNode->TargetType)
                {
                    FBPDependency Dep;
                    Dep.ReferencingBlueprint = SelfName;
                    Dep.ReferencedClass = CastNode->TargetType->GetName();
                    Dep.ReferenceType = TEXT("Cast");
                    Dep.NodeGuid = K2Node->NodeGuid.ToString();
                    Dep.GraphName = GraphName;
                    Out.Add(Dep);
                }
            }

            if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(K2Node))
            {
                if (UFunction* Fn = CallNode->GetTargetFunction())
                {
                    if (UClass* OwnerClass = Fn->GetOuterUClass())
                    {
                        const FString OwnerName = OwnerClass->GetName();
                        if (OwnerName != SelfName &&
                            OwnerName != TEXT("Object") &&
                            OwnerName != TEXT("KismetSystemLibrary") &&
                            OwnerName != TEXT("KismetMathLibrary") &&
                            OwnerName != TEXT("KismetStringLibrary") &&
                            OwnerName != TEXT("GameplayStatics"))
                        {
                            FBPDependency Dep;
                            Dep.ReferencingBlueprint = SelfName;
                            Dep.ReferencedClass = OwnerName;
                            Dep.ReferenceType = TEXT("Call");
                            Dep.NodeGuid = K2Node->NodeGuid.ToString();
                            Dep.GraphName = GraphName;
                            Out.Add(Dep);
                        }
                    }
                }
            }

            for (UEdGraphPin* Pin : K2Node->Pins)
            {
                if (Pin->Direction == EGPD_Input && Pin->DefaultObject)
                {
                    if (UBlueprint* RefBP = Cast<UBlueprint>(Pin->DefaultObject))
                    {
                        FBPDependency Dep;
                        Dep.ReferencingBlueprint = SelfName;
                        Dep.ReferencedClass = RefBP->GetName();
                        Dep.ReferenceType = TEXT("HardRef");
                        Dep.NodeGuid = K2Node->NodeGuid.ToString();
                        Dep.GraphName = GraphName;
                        Out.Add(Dep);
                    }
                }
            }
        }
    };

    for (UEdGraph* G : Blueprint->UbergraphPages) ProcessGraph(G);
    for (UEdGraph* G : Blueprint->FunctionGraphs) ProcessGraph(G);

    return Out;
}

static void FindDependencyCycles(
    const TMap<FString, TArray<FString>>& AdjacencyList,
    const FString& Start,
    TArray<FString>& OutChains)
{
    TSet<FString> Visited;
    TArray<FString> Path;

    TFunction<void(const FString&)> DFS = [&](const FString& Node)
    {
        Path.Add(Node);
        Visited.Add(Node);

        if (const TArray<FString>* Neighbors = AdjacencyList.Find(Node))
        {
            for (const FString& Next : *Neighbors)
            {
                if (Next == Start && Path.Num() >= 2)
                {
                    FString Chain = Path[0];
                    for (int32 i = 1; i < Path.Num(); ++i)
                    {
                        Chain += TEXT(" -> ") + Path[i];
                    }
                    Chain += TEXT(" -> ") + Start;
                    OutChains.AddUnique(Chain);
                }
                else if (!Visited.Contains(Next))
                {
                    DFS(Next);
                }
            }
        }

        Path.Pop();
    };

    DFS(Start);
}

FBPProjectAnalysis UBlueprintAnalyzerLibrary::AnalyzeFolder(const FString& FolderPath)
{
    FBPProjectAnalysis Result;
    Result.FolderPath = FolderPath;
    Result.AnalysisTimestamp = FDateTime::Now().ToString();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    FARFilter Filter;
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(FName(*FolderPath));

    TArray<FAssetData> Assets;
    AssetRegistry.GetAssets(Filter, Assets);

    int32 ScoreSum = 0;
    int32 NodeSum = 0;

    for (const FAssetData& AssetData : Assets)
    {
        UBlueprint* BP = Cast<UBlueprint>(AssetData.GetAsset());
        if (!BP) continue;

        FBPBlueprintSummary Summary;
        Summary.BlueprintName = BP->GetName();
        Summary.BlueprintPath = AssetData.GetObjectPathString();
        Summary.BlueprintType = GetBlueprintTypeString(BP);

        int32 NodeCount = 0;
        for (UEdGraph* G : BP->UbergraphPages)
        {
            if (G) NodeCount += G->Nodes.Num();
        }
        for (UEdGraph* G : BP->FunctionGraphs)
        {
            if (G) NodeCount += G->Nodes.Num();
        }
        Summary.NodeCount = NodeCount;
        NodeSum += NodeCount;

        FBPPerformanceReport PerfReport = AnalyzeBlueprintPerformance(BP);
        Summary.PerformanceScore = PerfReport.PerformanceScore;
        ScoreSum += PerfReport.PerformanceScore;
        for (const FBPPerformanceIssue& Issue : PerfReport.Issues)
        {
            if (Issue.Severity == EBPPerformanceSeverity::Critical)
            {
                Summary.CriticalIssues++;
            }
        }

        FBlueprintAnalysisResult Analysis = AnalyzeBlueprint(BP);
        const FString Text = ExportToLLMText(Analysis);
        Summary.EstimatedTokenCount = EstimateTokenCount(Text);

        Result.Dependencies.Append(ExtractBlueprintDependencies(BP));

        Result.Summaries.Add(Summary);
        Result.BlueprintsAnalyzed++;
    }

    Result.TotalNodes = NodeSum;
    Result.AveragePerformanceScore = Result.BlueprintsAnalyzed > 0
        ? static_cast<float>(ScoreSum) / static_cast<float>(Result.BlueprintsAnalyzed)
        : 100.0f;

    TMap<FString, TArray<FString>> Adjacency;
    for (const FBPDependency& Dep : Result.Dependencies)
    {
        Adjacency.FindOrAdd(Dep.ReferencingBlueprint).AddUnique(Dep.ReferencedClass);
    }
    for (const auto& Pair : Adjacency)
    {
        FindDependencyCycles(Adjacency, Pair.Key, Result.CircularDependencyChains);
    }

    return Result;
}

FString UBlueprintAnalyzerLibrary::ExportProjectAnalysisToJSON(const FBPProjectAnalysis& Analysis)
{
    TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
    Root->SetStringField(TEXT("FolderPath"), Analysis.FolderPath);
    Root->SetStringField(TEXT("AnalysisTimestamp"), Analysis.AnalysisTimestamp);
    Root->SetNumberField(TEXT("BlueprintsAnalyzed"), Analysis.BlueprintsAnalyzed);
    Root->SetNumberField(TEXT("TotalNodes"), Analysis.TotalNodes);
    Root->SetNumberField(TEXT("AveragePerformanceScore"), Analysis.AveragePerformanceScore);

    TArray<TSharedPtr<FJsonValue>> Summaries;
    for (const FBPBlueprintSummary& S : Analysis.Summaries)
    {
        TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject);
        O->SetStringField(TEXT("BlueprintName"), S.BlueprintName);
        O->SetStringField(TEXT("BlueprintPath"), S.BlueprintPath);
        O->SetStringField(TEXT("BlueprintType"), S.BlueprintType);
        O->SetNumberField(TEXT("NodeCount"), S.NodeCount);
        O->SetNumberField(TEXT("PerformanceScore"), S.PerformanceScore);
        O->SetNumberField(TEXT("CriticalIssues"), S.CriticalIssues);
        O->SetNumberField(TEXT("EstimatedTokenCount"), S.EstimatedTokenCount);
        Summaries.Add(MakeShareable(new FJsonValueObject(O)));
    }
    Root->SetArrayField(TEXT("Summaries"), Summaries);

    TArray<TSharedPtr<FJsonValue>> Deps;
    for (const FBPDependency& D : Analysis.Dependencies)
    {
        TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject);
        O->SetStringField(TEXT("ReferencingBlueprint"), D.ReferencingBlueprint);
        O->SetStringField(TEXT("ReferencedClass"), D.ReferencedClass);
        O->SetStringField(TEXT("ReferenceType"), D.ReferenceType);
        O->SetStringField(TEXT("NodeGuid"), D.NodeGuid);
        O->SetStringField(TEXT("GraphName"), D.GraphName);
        Deps.Add(MakeShareable(new FJsonValueObject(O)));
    }
    Root->SetArrayField(TEXT("Dependencies"), Deps);
    Root->SetArrayField(TEXT("CircularDependencyChains"), StringArrayToJson(Analysis.CircularDependencyChains));

    FString Out;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    return Out;
}

FString UBlueprintAnalyzerLibrary::ExportProjectAnalysisToLLMText(const FBPProjectAnalysis& Analysis)
{
    FString Out;
    Out += FString::Printf(TEXT("Project Analysis: %s\n"), *Analysis.FolderPath);
    Out += FString::Printf(TEXT("Analyzed at: %s\n\n"), *Analysis.AnalysisTimestamp);
    Out += TEXT("=== SUMMARY ===\n");
    Out += FString::Printf(TEXT("Blueprints analyzed: %d\n"), Analysis.BlueprintsAnalyzed);
    Out += FString::Printf(TEXT("Total nodes: %d\n"), Analysis.TotalNodes);
    Out += FString::Printf(TEXT("Average performance score: %.1f/100\n\n"), Analysis.AveragePerformanceScore);

    TArray<FBPBlueprintSummary> SortedSummaries = Analysis.Summaries;
    SortedSummaries.Sort([](const FBPBlueprintSummary& A, const FBPBlueprintSummary& B)
    {
        return A.PerformanceScore < B.PerformanceScore;
    });

    Out += TEXT("=== TOP OFFENDERS (worst performance first) ===\n");
    const int32 MaxShown = FMath::Min(SortedSummaries.Num(), 10);
    for (int32 i = 0; i < MaxShown; ++i)
    {
        const FBPBlueprintSummary& S = SortedSummaries[i];
        Out += FString::Printf(TEXT("%d. %s (%s) - score %d/100, %d critical, %d nodes, ~%d tokens\n"),
            i + 1, *S.BlueprintName, *S.BlueprintType, S.PerformanceScore, S.CriticalIssues, S.NodeCount, S.EstimatedTokenCount);
    }
    Out += TEXT("\n");

    if (Analysis.CircularDependencyChains.Num() > 0)
    {
        Out += TEXT("=== CIRCULAR DEPENDENCIES ===\n");
        for (const FString& Chain : Analysis.CircularDependencyChains)
        {
            Out += FString::Printf(TEXT("- %s\n"), *Chain);
        }
        Out += TEXT("\n");
    }

    Out += FString::Printf(TEXT("=== DEPENDENCIES (%d total) ===\n"), Analysis.Dependencies.Num());
    for (const FBPDependency& D : Analysis.Dependencies)
    {
        Out += FString::Printf(TEXT("- %s --(%s)--> %s  [graph: %s]\n"),
            *D.ReferencingBlueprint, *D.ReferenceType, *D.ReferencedClass, *D.GraphName);
    }

    return Out;
}

// Helper: converts TArray<FString> to JSON array of strings
static TArray<TSharedPtr<FJsonValue>> StringArrayToJson(const TArray<FString>& InArray)
{
    TArray<TSharedPtr<FJsonValue>> Out;
    for (const FString& S : InArray)
    {
        Out.Add(MakeShareable(new FJsonValueString(S)));
    }
    return Out;
}

// Helper: converts FBPFunctionParam array to JSON
static TArray<TSharedPtr<FJsonValue>> ParamsToJson(const TArray<FBPFunctionParam>& Params)
{
    TArray<TSharedPtr<FJsonValue>> Out;
    for (const FBPFunctionParam& P : Params)
    {
        TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject);
        O->SetStringField(TEXT("ParamName"), P.ParamName);
        O->SetStringField(TEXT("ParamType"), P.ParamType);
        O->SetBoolField(TEXT("IsReference"), P.bIsReference);
        O->SetBoolField(TEXT("IsConst"), P.bIsConst);
        O->SetBoolField(TEXT("IsReturn"), P.bIsReturn);
        Out.Add(MakeShareable(new FJsonValueObject(O)));
    }
    return Out;
}

static TSharedPtr<FJsonObject> MetadataToJson(const FBPAnalyzerMetadata& Meta)
{
    TSharedPtr<FJsonObject> MetaObject = MakeShareable(new FJsonObject);
    MetaObject->SetStringField(TEXT("BlueprintType"), Meta.BlueprintType);
    MetaObject->SetStringField(TEXT("ParentClass"), Meta.ParentClass);
    MetaObject->SetArrayField(TEXT("ImplementedInterfaces"), StringArrayToJson(Meta.ImplementedInterfaces));
    MetaObject->SetArrayField(TEXT("MacroNames"), StringArrayToJson(Meta.MacroNames));
    MetaObject->SetArrayField(TEXT("TimelineNames"), StringArrayToJson(Meta.TimelineNames));

    TArray<TSharedPtr<FJsonValue>> VarsJson;
    for (const FBPVariableInfo& Var : Meta.Variables)
    {
        TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject);
        O->SetStringField(TEXT("VariableName"), Var.VariableName);
        O->SetStringField(TEXT("VariableType"), Var.VariableType);
        O->SetStringField(TEXT("DefaultValue"), Var.DefaultValue);
        O->SetStringField(TEXT("Category"), Var.Category);
        O->SetStringField(TEXT("Tooltip"), Var.Tooltip);
        O->SetBoolField(TEXT("Editable"), Var.bEditable);
        O->SetBoolField(TEXT("Replicated"), Var.bReplicated);
        O->SetBoolField(TEXT("ExposeOnSpawn"), Var.bExposeOnSpawn);
        VarsJson.Add(MakeShareable(new FJsonValueObject(O)));
    }
    MetaObject->SetArrayField(TEXT("Variables"), VarsJson);

    TArray<TSharedPtr<FJsonValue>> FuncsJson;
    for (const FBPFunctionSignature& Fn : Meta.CustomFunctions)
    {
        TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject);
        O->SetStringField(TEXT("FunctionName"), Fn.FunctionName);
        O->SetStringField(TEXT("ReturnType"), Fn.ReturnType);
        O->SetStringField(TEXT("AccessSpecifier"), Fn.AccessSpecifier);
        O->SetStringField(TEXT("Category"), Fn.Category);
        O->SetBoolField(TEXT("Pure"), Fn.bPure);
        O->SetBoolField(TEXT("Static"), Fn.bStatic);
        O->SetBoolField(TEXT("Const"), Fn.bConst);
        O->SetArrayField(TEXT("Parameters"), ParamsToJson(Fn.Parameters));
        FuncsJson.Add(MakeShareable(new FJsonValueObject(O)));
    }
    MetaObject->SetArrayField(TEXT("CustomFunctions"), FuncsJson);

    TArray<TSharedPtr<FJsonValue>> CompsJson;
    for (const FBPComponentInfo& Comp : Meta.Components)
    {
        TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject);
        O->SetStringField(TEXT("ComponentName"), Comp.ComponentName);
        O->SetStringField(TEXT("ComponentType"), Comp.ComponentType);
        O->SetStringField(TEXT("ParentComponentName"), Comp.ParentComponentName);
        O->SetStringField(TEXT("AttachSocketName"), Comp.AttachSocketName);
        CompsJson.Add(MakeShareable(new FJsonValueObject(O)));
    }
    MetaObject->SetArrayField(TEXT("Components"), CompsJson);

    TArray<TSharedPtr<FJsonValue>> DispatchersJson;
    for (const FBPEventDispatcherInfo& Dispatcher : Meta.EventDispatchers)
    {
        TSharedPtr<FJsonObject> O = MakeShareable(new FJsonObject);
        O->SetStringField(TEXT("DispatcherName"), Dispatcher.DispatcherName);
        O->SetArrayField(TEXT("Parameters"), ParamsToJson(Dispatcher.Parameters));
        DispatchersJson.Add(MakeShareable(new FJsonValueObject(O)));
    }
    MetaObject->SetArrayField(TEXT("EventDispatchers"), DispatchersJson);

    return MetaObject;
}

FString UBlueprintAnalyzerLibrary::ExportToJSON(const FBlueprintAnalysisResult& AnalysisResult)
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);

    RootObject->SetStringField(TEXT("BlueprintName"), AnalysisResult.BlueprintName);
    RootObject->SetStringField(TEXT("AnalysisTimestamp"), AnalysisResult.AnalysisTimestamp);
    RootObject->SetObjectField(TEXT("Metadata"), MetadataToJson(AnalysisResult.Metadata));

    TArray<TSharedPtr<FJsonValue>> NodesArray;
    for (const FBlueprintNodeInfo& Node : AnalysisResult.Nodes)
    {
        TSharedPtr<FJsonObject> NodeObject = MakeShareable(new FJsonObject);
        NodeObject->SetStringField(TEXT("NodeGuid"), Node.NodeGuid);
        NodeObject->SetStringField(TEXT("NodeType"), Node.NodeType);
        NodeObject->SetStringField(TEXT("NodeName"), Node.NodeName);
        NodeObject->SetStringField(TEXT("FunctionName"), Node.FunctionName);
        NodeObject->SetStringField(TEXT("GraphName"), Node.GraphName);
        NodeObject->SetStringField(TEXT("CommentGroup"), Node.CommentGroup);
        NodeObject->SetArrayField(TEXT("InputPins"), StringArrayToJson(Node.InputPins));
        NodeObject->SetArrayField(TEXT("OutputPins"), StringArrayToJson(Node.OutputPins));
        NodeObject->SetArrayField(TEXT("LiteralValues"), StringArrayToJson(Node.LiteralValues));
        NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObject)));
    }
    RootObject->SetArrayField(TEXT("Nodes"), NodesArray);

    // Execution paths
    TArray<TSharedPtr<FJsonValue>> PathsArray;
    for (const FExecutionPath& Path : AnalysisResult.ExecutionPaths)
    {
        TSharedPtr<FJsonObject> PathObject = MakeShareable(new FJsonObject);
        PathObject->SetStringField(TEXT("EntryPointName"), Path.EntryPointName);
        PathObject->SetStringField(TEXT("EntryNodeGuid"), Path.EntryNodeGuid);
        PathObject->SetStringField(TEXT("GraphName"), Path.GraphName);

        TArray<TSharedPtr<FJsonValue>> StepsArray;
        for (const FExecutionStep& Step : Path.Steps)
        {
            TSharedPtr<FJsonObject> StepObject = MakeShareable(new FJsonObject);
            StepObject->SetStringField(TEXT("NodeGuid"), Step.NodeGuid);
            StepObject->SetStringField(TEXT("NodeType"), Step.NodeType);
            StepObject->SetStringField(TEXT("Summary"), Step.Summary);
            StepObject->SetStringField(TEXT("BranchLabel"), Step.BranchLabel);
            StepObject->SetNumberField(TEXT("Depth"), Step.Depth);
            StepObject->SetBoolField(TEXT("IsTerminator"), Step.bIsTerminator);
            StepObject->SetBoolField(TEXT("IsLatent"), Step.bIsLatent);
            StepsArray.Add(MakeShareable(new FJsonValueObject(StepObject)));
        }
        PathObject->SetArrayField(TEXT("Steps"), StepsArray);
        PathsArray.Add(MakeShareable(new FJsonValueObject(PathObject)));
    }
    RootObject->SetArrayField(TEXT("ExecutionPaths"), PathsArray);

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

    // Metadata section
    const FBPAnalyzerMetadata& Meta = AnalysisResult.Metadata;
    Result += TEXT("=== METADATA ===\n");
    Result += FString::Printf(TEXT("BlueprintType: %s\n"), *Meta.BlueprintType);
    Result += FString::Printf(TEXT("ParentClass: %s\n"), *Meta.ParentClass);
    if (Meta.ImplementedInterfaces.Num() > 0)
    {
        Result += FString::Printf(TEXT("Interfaces: %s\n"), *FString::Join(Meta.ImplementedInterfaces, TEXT(", ")));
    }
    Result += TEXT("\n");

    if (Meta.Components.Num() > 0)
    {
        Result += TEXT("--- Components ---\n");
        for (const FBPComponentInfo& Comp : Meta.Components)
        {
            Result += FString::Printf(TEXT("- %s (%s)"), *Comp.ComponentName, *Comp.ComponentType);
            if (!Comp.ParentComponentName.IsEmpty())
            {
                Result += FString::Printf(TEXT(" attached to %s"), *Comp.ParentComponentName);
            }
            Result += TEXT("\n");
        }
        Result += TEXT("\n");
    }

    if (Meta.Variables.Num() > 0)
    {
        Result += TEXT("--- Variables ---\n");
        for (const FBPVariableInfo& Var : Meta.Variables)
        {
            FString Flags;
            if (Var.bEditable) Flags += TEXT(" Editable");
            if (Var.bReplicated) Flags += TEXT(" Replicated");
            if (Var.bExposeOnSpawn) Flags += TEXT(" ExposeOnSpawn");
            Result += FString::Printf(TEXT("- %s : %s"), *Var.VariableName, *Var.VariableType);
            if (!Var.DefaultValue.IsEmpty()) Result += FString::Printf(TEXT(" = %s"), *Var.DefaultValue);
            if (!Flags.IsEmpty()) Result += FString::Printf(TEXT(" [%s ]"), *Flags);
            if (!Var.Category.IsEmpty() && Var.Category != TEXT("Default")) Result += FString::Printf(TEXT(" (Category: %s)"), *Var.Category);
            Result += TEXT("\n");
        }
        Result += TEXT("\n");
    }

    if (Meta.CustomFunctions.Num() > 0)
    {
        Result += TEXT("--- Custom Functions ---\n");
        for (const FBPFunctionSignature& Fn : Meta.CustomFunctions)
        {
            FString ParamsStr;
            for (int32 i = 0; i < Fn.Parameters.Num(); ++i)
            {
                if (i > 0) ParamsStr += TEXT(", ");
                ParamsStr += FString::Printf(TEXT("%s %s"), *Fn.Parameters[i].ParamType, *Fn.Parameters[i].ParamName);
            }
            const FString ReturnType = Fn.ReturnType.IsEmpty() ? TEXT("void") : Fn.ReturnType;
            Result += FString::Printf(TEXT("- %s %s(%s)"), *ReturnType, *Fn.FunctionName, *ParamsStr);
            if (Fn.bPure) Result += TEXT(" [Pure]");
            if (Fn.bStatic) Result += TEXT(" [Static]");
            if (Fn.bConst) Result += TEXT(" [Const]");
            if (!Fn.AccessSpecifier.IsEmpty() && Fn.AccessSpecifier != TEXT("Public")) Result += FString::Printf(TEXT(" [%s]"), *Fn.AccessSpecifier);
            Result += TEXT("\n");
        }
        Result += TEXT("\n");
    }

    if (Meta.EventDispatchers.Num() > 0)
    {
        Result += TEXT("--- Event Dispatchers ---\n");
        for (const FBPEventDispatcherInfo& Dispatcher : Meta.EventDispatchers)
        {
            FString ParamsStr;
            for (int32 i = 0; i < Dispatcher.Parameters.Num(); ++i)
            {
                if (i > 0) ParamsStr += TEXT(", ");
                ParamsStr += FString::Printf(TEXT("%s %s"), *Dispatcher.Parameters[i].ParamType, *Dispatcher.Parameters[i].ParamName);
            }
            Result += FString::Printf(TEXT("- %s(%s)\n"), *Dispatcher.DispatcherName, *ParamsStr);
        }
        Result += TEXT("\n");
    }

    if (Meta.MacroNames.Num() > 0)
    {
        Result += FString::Printf(TEXT("Macros: %s\n\n"), *FString::Join(Meta.MacroNames, TEXT(", ")));
    }
    if (Meta.TimelineNames.Num() > 0)
    {
        Result += FString::Printf(TEXT("Timelines: %s\n\n"), *FString::Join(Meta.TimelineNames, TEXT(", ")));
    }

    Result += TEXT("=== NODES ===\n");
    for (const FBlueprintNodeInfo& Node : AnalysisResult.Nodes)
    {
        Result += FString::Printf(TEXT("- %s [%s]: %s"), *Node.NodeType, *Node.NodeGuid, *Node.NodeName);
        if (!Node.GraphName.IsEmpty())
        {
            Result += FString::Printf(TEXT("  (in graph: %s)"), *Node.GraphName);
        }
        Result += TEXT("\n");
        if (!Node.FunctionName.IsEmpty())
        {
            Result += FString::Printf(TEXT("  Function: %s\n"), *Node.FunctionName);
        }
        if (Node.InputPins.Num() > 0)
        {
            Result += FString::Printf(TEXT("  Inputs: %s\n"), *FString::Join(Node.InputPins, TEXT(", ")));
        }
        if (Node.OutputPins.Num() > 0)
        {
            Result += FString::Printf(TEXT("  Outputs: %s\n"), *FString::Join(Node.OutputPins, TEXT(", ")));
        }
        if (Node.LiteralValues.Num() > 0)
        {
            Result += FString::Printf(TEXT("  Literals: %s\n"), *FString::Join(Node.LiteralValues, TEXT(", ")));
        }
        if (!Node.CommentGroup.IsEmpty())
        {
            Result += FString::Printf(TEXT("  Comment: %s\n"), *Node.CommentGroup);
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

    // Execution paths — tree-like indented view for LLM consumption
    if (AnalysisResult.ExecutionPaths.Num() > 0)
    {
        Result += TEXT("\n=== EXECUTION FLOW ===\n");
        for (const FExecutionPath& Path : AnalysisResult.ExecutionPaths)
        {
            Result += FString::Printf(TEXT("\n[%s] in %s\n"), *Path.EntryPointName, *Path.GraphName);
            for (const FExecutionStep& Step : Path.Steps)
            {
                const FString Indent = FString::ChrN(Step.Depth * 2, ' ');
                FString Prefix;
                if (!Step.BranchLabel.IsEmpty())
                {
                    Prefix = FString::Printf(TEXT("[%s] "), *Step.BranchLabel);
                }
                FString Suffix;
                if (Step.bIsLatent) Suffix += TEXT(" (latent)");
                if (Step.bIsTerminator) Suffix += TEXT(" (cycle)");
                Result += FString::Printf(TEXT("%s- %s%s%s\n"), *Indent, *Prefix, *Step.Summary, *Suffix);
            }
        }
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

                // Phase 5: replace heuristic bindings with actual UWidgetBlueprint::Bindings
                TMap<FString, TArray<FString>> RealBindingsByWidget;
                for (const FDelegateEditorBinding& Binding : WidgetBP->Bindings)
                {
                    RealBindingsByWidget.FindOrAdd(Binding.ObjectName).Add(Binding.PropertyName.ToString());
                }
                for (FWidgetHierarchyInfo& WidgetInfo : Report.WidgetHierarchy)
                {
                    if (TArray<FString>* Props = RealBindingsByWidget.Find(WidgetInfo.WidgetName))
                    {
                        WidgetInfo.BoundProperties = *Props;
                        WidgetInfo.bHasBindings = Props->Num() > 0;
                    }
                    else
                    {
                        WidgetInfo.BoundProperties.Reset();
                        WidgetInfo.bHasBindings = false;
                    }
                }

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
