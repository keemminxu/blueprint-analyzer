![img](https://github.com/keemminxu/blueprint-analyzer/blob/0dc6b752c9fe89d79d98671b0b795c858facea01/Thumbnail/BlueprintAnalyzer_0.png)

# Blueprint Analyzer Plugin

A comprehensive Unreal Engine 5 editor plugin that converts .uasset Blueprint files to LLM-friendly formats and provides advanced UMG Widget optimization analysis. Perfect for AI-assisted development, code review, and performance optimization.

## ✨ Key Features

### Blueprint to LLM Conversion
- **🔄 .uasset to Text Conversion**: Convert binary Blueprint files to readable text format
- **🔍 Complete Structure Analysis**: Extract all node and connection information from Blueprint graphs
- **📊 Multiple Export Formats**: JSON for programmatic use, Human-readable text for LLM analysis  
- **⚡ Massive Data Reduction**: 80-90% smaller output by removing visual layout information
- **🎯 AI-Optimized Output**: Specifically formatted for ChatGPT, Claude, and other AI assistants
- **🌐 Universal Blueprint Support**: Works with all Blueprint types (Actor, Component, Interface, etc.)

### Widget Blueprint Optimization
- **🎨 UMG Performance Analysis**: Comprehensive Widget Blueprint optimization scoring (0-100 points)
- **📏 Hierarchy Analysis**: Detect deep nesting issues (>5 levels) and layout complexity
- **🎯 Smart Optimization Detection**: Identify missing Invalidation Box and Retainer Box usage
- **⚙️ Widget Combination Analysis**: Detect problematic ScaleBox + SizeBox combinations
- **📈 Performance Recommendations**: Get specific, actionable optimization suggestions

### Universal Support
- **🔧 Editor Integration**: Right-click context menu integration in Content Browser
- **🌐 Universal Blueprint Support**: Works with regular Blueprints, Widget Blueprints, Animation Blueprints, etc.

## 🚀 Installation

1. Copy the `BlueprintAnalyzer` folder to your project's `Plugins` directory
2. Regenerate project files
3. Build your project
4. Enable the plugin in Editor → Plugins → Developer Tools → Blueprint Analyzer

## 📋 Usage

### Blueprint to LLM Conversion

#### Quick Blueprint Analysis
1. Select any Blueprint (.uasset) in the Content Browser
2. Right-click to open context menu
3. Choose **Blueprint Analyzer** → **Analyze Blueprint**
4. View node count and connection summary in popup

#### Convert Blueprint to JSON
1. Select a Blueprint in the Content Browser
2. Right-click → **Blueprint Analyzer** → **Export to JSON**
3. Choose save location for the .json file
4. Use the structured data for:
   - Programmatic analysis
   - Integration with other tools
   - Automated Blueprint processing

#### Convert Blueprint for AI Analysis
1. Select a Blueprint in the Content Browser
2. Right-click → **Blueprint Analyzer** → **Export to LLM Text**
3. Choose save location for the .txt file
4. Copy the content and paste into:
   - ChatGPT for code review and suggestions
   - Claude for Blueprint analysis and C++ conversion
   - Any LLM for documentation generation

### Widget Blueprint Optimization

#### Quick Optimization Analysis
1. Select a Widget Blueprint in the Content Browser
2. Right-click to open context menu
3. Choose **Blueprint Analyzer** → **Analyze Widget Blueprint**
4. View optimization score and recommendations in popup dialog

#### Detailed Optimization Report
1. Select a Widget Blueprint → Right-click → **Blueprint Analyzer**
2. Choose **Export Widget Analysis to JSON** or **Export Widget Analysis to LLM Text**
3. Get detailed analysis including:
   - Widget hierarchy breakdown
   - Specific optimization issues
   - Performance recommendations
   - Memory usage estimates

## 📄 Output Examples

### Blueprint Analysis JSON Format
```json
{
  "BlueprintName": "BP_PlayerCharacter",
  "AnalysisTimestamp": "2024-01-15 14:30:22",
  "Nodes": [
    {
      "NodeType": "Event",
      "NodeName": "BeginPlay",
      "FunctionName": "",
      "InputPins": [],
      "OutputPins": ["exec:exec"]
    }
  ],
  "Connections": [
    {
      "FromNodeGuid": "ABC123",
      "FromPinName": "exec",
      "ToNodeGuid": "DEF456",
      "ToPinName": "exec"
    }
  ]
}
```

### LLM-Friendly Text Format
```
Blueprint Analysis: BP_PlayerCharacter
Analyzed at: 2024-01-15 14:30:22

=== NODES ===
- Event [ABC123]: BeginPlay
  Outputs: exec:exec

- FunctionCall [DEF456]: Set Actor Location
  Function: SetActorLocation
  Inputs: exec:exec, NewLocation:vector
  Outputs: exec:exec

=== CONNECTIONS ===
ABC123.exec -> DEF456.exec
```

### Widget Blueprint Optimization Report
```
Widget Blueprint Optimization Report: WBP_MainMenu
Analyzed at: 2024-01-15 14:30:22

=== SUMMARY ===
Total Widgets: 15
Maximum Depth: 6
Total Bindings: 4
Estimated Memory Usage: 23.50 KB
Optimization Score: 70/100

=== OPTIMIZATION ISSUES ===
[WARNING] Deep Nesting Beyond 5 Levels: Widget 'Button_Submit' is nested 6 levels deep (max recommended: 5)
  Widget: Button_Submit
  Recommendation: Flatten widget hierarchy to improve layout calculation performance (-10 points)

[CRITICAL] Missing Invalidation Box: Dynamic widgets (TextBlock) found but no Invalidation Box is used
  Widget: Root
  Recommendation: Wrap frequently updating widgets with Invalidation Box for better rendering performance (-20 points)
```

## 🎯 Use Cases

### AI-Assisted Development (.uasset → LLM)
- **Blueprint to Text Conversion**: Transform binary .uasset files into LLM-readable format
- **Code Review with AI**: Share blueprint logic with ChatGPT/Claude for analysis and suggestions
- **Blueprint to C++ Migration**: Get AI assistance to convert Blueprint logic to optimized C++ code
- **Auto Documentation**: Generate comprehensive documentation for complex Blueprint systems
- **Logic Debugging**: Use AI to identify potential issues, infinite loops, or logic flaws
- **Architecture Analysis**: Get AI feedback on Blueprint architecture and design patterns

### UMG Performance Optimization
- **Performance Audits**: Quickly identify performance bottlenecks in Widget Blueprints
- **Best Practice Enforcement**: Ensure teams follow UMG optimization guidelines
- **Memory Optimization**: Get estimates and recommendations for memory usage reduction
- **CI/CD Integration**: Automate widget performance checks in your build pipeline

### Team Collaboration & Workflow
- **Version Control Friendly**: Track Blueprint changes in readable text format instead of binary diffs
- **Remote Code Reviews**: Review Blueprint logic without needing Unreal Editor access
- **Cross-Team Communication**: Share Blueprint functionality with non-technical team members (designers, QA)
- **Legacy System Analysis**: Convert old Blueprints to text for easier migration planning
- **Performance Standards**: Establish and maintain UI performance standards across projects
- **External Tool Integration**: Feed Blueprint data into custom analysis tools and pipelines

## 🔧 Technical Details

### Supported Node Types
- Events (BeginPlay, Tick, Input Events, etc.)
- Function Calls
- Variable Get/Set operations
- Control Flow (Branch, Loop, etc.)
- Mathematical Operations
- Custom Events
- And many more...

### Blueprint Types Supported
- Regular Blueprints
- Widget Blueprints
- Animation Blueprints  
- Actor Blueprints
- Component Blueprints
- Interface Blueprints

### Blueprint Data Extraction & Reduction
The plugin intelligently converts binary .uasset files to text while preserving essential logic:

#### What's Removed (Visual/Editor-Only Data)
- ❌ Node positions (X, Y coordinates)
- ❌ Visual styling and colors
- ❌ Comment box positions and formatting
- ❌ Complex GUID details (simplified for readability)
- ❌ Editor-specific metadata
- ❌ Thumbnail and preview data

#### What's Preserved (Logic & Structure)
- ✅ Complete node types and names
- ✅ Function calls with all parameters
- ✅ Variable get/set operations
- ✅ Event handling and execution flow
- ✅ Pin types and data connections
- ✅ Control flow (branches, loops, gates)
- ✅ Blueprint inheritance and interfaces
- ✅ Custom events and delegates

#### Result: 80-90% Size Reduction
- **Before**: 500KB+ binary .uasset file
- **After**: 50-100KB readable text file
- **Benefit**: Perfect for LLM context windows and version control

### Widget Blueprint Optimization Scoring

The plugin uses a comprehensive 100-point scoring system for Widget Blueprint optimization:

#### Deduction Criteria
- **Deep Nesting (-10 points)**: Widget hierarchy exceeds 5 levels
- **Scale Box + Size Box Combination (-5 points)**: These widgets used together in parent-child relationship
- **Missing Invalidation Box (-20 points)**: Dynamic widgets without proper invalidation caching
- **Missing Retainer Box (-10 points)**: Complex static widgets without render caching

#### Bonus Points
- **Optimization Box Active Usage (+20 points)**: Proper use of both Invalidation and Retainer boxes

#### Score Grades
- **90-100**: Excellent! - Very Well Optimized
- **70-89**: Good - Well Optimized  
- **50-69**: Average - Some Improvements Needed
- **30-49**: Poor - Significant Optimization Required
- **0-29**: Critical - Complete Redesign Required

For detailed scoring documentation, see [UMG_OPTIMIZATION_SCORING.md](UMG_OPTIMIZATION_SCORING.md).

## ⚙️ System Requirements

- **Unreal Engine**: 5.5 or later
- **Platform**: Windows
- **Build Tools**: Visual Studio 2022
- **Editor Only**: This plugin only works in the Unreal Editor

## 🤝 Contributing

We welcome contributions! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/keemminxu/blueprint-analyzer/issues)
- **Discussions**: [GitHub Discussions](https://github.com/keemminxu/blueprint-analyzer/discussions)

## 📜 License

This project is provided free of charge for use in Unreal Engine projects - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Epic Games for the Unreal Engine
- The UE developer community for inspiration and feedback
- All contributors who helped improve this plugin

---

**Made with ❤️ by keemminxu**