$content = Get-Content 'source/preset/FactoryPresetCatalog.h' -Raw
$pattern = 'catalog\.push_back\(\{\s*"([^"]+)",\s*"([^"]+)",\s*R"json\((.*?)\)json"'
$regex = [regex]::new($pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)
$matches = $regex.Matches($content)

$report = @()
$idx = 0
foreach ($m in $matches) {
    $idx++
    $name = $m.Groups[1].Value
    $cat = $m.Groups[2].Value
    $json = $m.Groups[3].Value
    
    # Extract node names
    $nodeNames = [regex]::Matches($json, '"name":\s*"([^"]+)"') | ForEach-Object { $_.Groups[1].Value }
    # Skip preset name which is the first one
    $nodes = $nodeNames[1..($nodeNames.Count-1)] -join " -> "
    
    # Count connections
    $connCount = ([regex]::Matches($json, '"srcNode":')).Count
    $nodeCount = ([regex]::Matches($json, '"type":')).Count
    
    # Check if purely linear
    $isLinear = ($connCount -eq ($nodeCount - 1))
    
    $report += [PSCustomObject]@{
        Index = $idx
        Name = $name
        Category = $cat
        NodesCount = $nodeCount
        ConnsCount = $connCount
        IsLinear = $isLinear
        Topology = $nodes
    }
}

$report | Format-Table -AutoSize Index, Name, Category, NodesCount, ConnsCount, IsLinear | Out-String -Width 140
