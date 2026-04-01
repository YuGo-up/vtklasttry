# 网格质量评估实验报告

## 基本信息

- 时间：{{TIME}}
- 数据源：{{SOURCE_PATH}}
- 点/单元：{{POINTS}} / {{CELLS}}
- 导入管线：{{IMPORT_PIPELINE}}

## 网格分类结果

- 分类判定：{{MESH_CLASS}}

| cell_type | cell_type_name | count | ratio |
|---:|---|---:|---:|
{{CELLTYPE_STATS_TABLE}}

### 常见 VTK 单元类型对照表

| VTK 类型 | 枚举值 | 描述 | 节点数 | 几何形状 |
|---|---:|---|---:|---|
| VTK_VERTEX | 1 | 点单元 | 1 | 点 |
| VTK_LINE | 3 | 线单元 | 2 | 线段 |
| VTK_TRIANGLE | 5 | 三角形 | 3 | 三角形面 |
| VTK_QUAD | 9 | 四边形 | 4 | 四边形面 |
| VTK_TETRA | 10 | 四面体 | 4 | 四面体体单元 |
| VTK_HEXAHEDRON | 12 | 六面体 | 8 | 六面体体单元 |

## 质量指标

- metric_key：`{{METRIC_KEY}}`
- 指标名称：{{METRIC_NAME}}
- 指标范围：[{{RANGE_MIN}}, {{RANGE_MAX}}]
- 指标适配：{{METRIC_SUPPORT}}

## 坏单元判定

- 阈值模板：{{TEMPLATE_TEXT}}（`{{TEMPLATE_KEY}}`）
- 判定规则：{{RULE_TEXT}}（`{{RULE_KEY}}`）
- 阈值：{{THRESHOLD}}
- 组合规则：{{COMBO_RULE}}
- 坏单元统计：{{BAD}} / {{TOTAL}}（{{BAD_RATIO}}%）

## 导出文件（可选）

- CSV：{{CSV_PATH}}
- PNG：{{PNG_PATH}}

## 坏单元示例（Top 20）

| cell_id | cell_type | cell_type_name | quality |
|---:|---:|---|---:|
{{BAD_TABLE}}

### 字段解释

- **cell_id**：单元在当前网格中的索引（从 0 开始），用于定位与复现。
- **cell_type**：VTK 的单元类型编号（`vtkCellType`），用于区分三角形/四边形/四面体/六面体等。
- **cell_type_name**：`cell_type` 对应的 VTK 单元类型名称（例如 `VTK_QUAD`、`VTK_TRIANGLE` 等）。
- **quality**：当前所选质量指标在该单元上的数值（与 `metric_key / 指标名称` 对应）。
  - 解释：{{QUALITY_EXPLAIN}}

