# UMG Widget Blueprint Optimization Scoring System

This scoring system evaluates UMG Widget Blueprint optimization on a scale of 0-100 points, designed to intuitively identify issues and provide improvement guidance.

## Scoring Criteria

### Deduction Items

#### 1. Widget Hierarchy Structure (-10 points)
- **Trigger**: Widget nesting depth exceeds 5 levels
- **Reason**: Deep hierarchies increase layout calculation complexity
- **Detection**: Any widget with Depth > 5
- **Recommendation**: Flatten widget hierarchy to improve performance

#### 2. Scale Box + Size Box Combination (-5 points)
- **Trigger**: ScaleBox and SizeBox used together in parent-child relationship
- **Reason**: This combination can cause unnecessary calculations
- **Detection**: SizeBox found as child of ScaleBox in hierarchy
- **Recommendation**: Avoid using ScaleBox and SizeBox together

#### 3. Missing Invalidation Box (-20 points)
- **Trigger**: Dynamic widgets present without Invalidation Box
- **Dynamic Widgets**: ProgressBar, TextBlock, Slider
- **Reason**: Frequent updates without proper invalidation hurt performance
- **Recommendation**: Wrap frequently updating widgets with Invalidation Box

#### 4. Missing Retainer Box (-10 points)
- **Trigger**: Complex static widgets without Retainer Box
- **Complex Widgets**: Widgets with >10 children or Image widgets
- **Reason**: Complex static content benefits from render caching
- **Recommendation**: Use Retainer Box for complex static widget groups

### Bonus Points

#### Optimization Box Active Usage (+20 points)
- **Trigger**: Appropriate use of both Invalidation Box and Retainer Box
- **Condition**: No missing optimization box issues AND total widgets > 5
- **Reason**: Demonstrates proper understanding and application of UMG optimization
- **Maximum Score**: Capped at 100 points

## Evaluation Grades

| Score Range | Grade | Description |
|-------------|-------|-------------|
| 90-100 | **Excellent** | Very Well Optimized |
| 70-89 | **Good** | Well Optimized |
| 50-69 | **Average** | Some Improvements Needed |
| 30-49 | **Poor** | Significant Optimization Required |
| 0-29 | **Critical** | Complete Redesign Required |

## Analysis Metrics

### Total Widgets
- **Definition**: Count of all widgets in the widget tree
- **Purpose**: Understanding widget complexity

### Max Depth
- **Definition**: Maximum nesting level in widget hierarchy
- **Purpose**: Identifying hierarchy complexity issues

### Total Bindings
- **Definition**: Count of bound properties across all widgets
- **Source**: Reads the actual `UWidgetBlueprint::Bindings` array, not heuristics
- **Purpose**: Understanding binding complexity for performance impact assessment

### Optimization Issues
- **Definition**: Number of detected optimization problems
- **Types**: Deep Nesting, Box Combinations, Missing Optimization Boxes

### Estimated Memory Usage
- **Definition**: Rough memory estimation based on widget types
- **Calculation**: Base widget memory + type-specific additions + binding overhead

## Implementation Notes

### Widget Detection
- Uses widget type name matching (e.g., "InvalidationBox", "RetainerBox")
- Hierarchy traversal to detect parent-child relationships
- Binding detection via `UWidgetBlueprint::Bindings` — the same source the editor uses

### Scoring Algorithm
1. Start with 100 points
2. Apply deductions for each detected issue
3. Check for bonus conditions
4. Clamp result to 0-100 range

### Recommendations Priority
1. Missing Invalidation Box (Critical, -20 points)
2. Deep Nesting (High, -10 points)
3. Missing Retainer Box (Medium, -10 points)
4. Box Combinations (Low, -5 points)

## Usage

The analyzer is accessible through the Content Browser context menu:
1. Right-click on a Widget Blueprint asset
2. Select "Blueprint Analyzer" → "Analyze Widget Blueprint"
3. View results in popup dialog
4. Export detailed reports via JSON or LLM-friendly text formats

## Related: Blueprint Performance Scoring

In addition to this UMG-specific scoring system, the plugin ships a parallel **Blueprint Performance Scoring** system for regular (non-Widget) Blueprints. It applies the same 0-100 grading philosophy to classic performance anti-patterns:

| Issue | Deduction |
|-------|-----------|
| Expensive call in Tick (`GetAllActorsOfClass`, `LineTrace`, etc.) | -25 (up to 3 occurrences) |
| Cast in Tick | -10 (up to 3 occurrences) |
| Heavy Tick graph (>50 downstream nodes) | -15 |
| Bloated BeginPlay (>100 downstream nodes) | -10 |
| Excessive Cast usage (>20 total) | -5 |

Access via Content Browser right-click on any Blueprint → **Performance Analysis** → **Analyze Blueprint Performance**. Also available in batch mode through **Analyze Folder** for project-wide audits.

## Future Improvements

- More sophisticated parent-child relationship tracking (currently depth-based approximation for ScaleBox/SizeBox detection)
- Precise per-widget memory measurement (currently heuristic-based estimation)
- Integration with UE5 Slate Insights / profiling tools
- Custom optimization rule definitions via project settings
- Animation-aware optimization rules (UMG Animation cost detection)