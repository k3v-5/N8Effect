$content = Get-Content 'source/preset/FactoryPresetCatalog.h' -Raw
$pattern = 'catalog\.push_back\(\{\s*"([^"]+)",\s*"([^"]+)",\s*R"json\((.*?)\)json"'
$regex = [regex]::new($pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)
$matches = $regex.Matches($content)
Write-Host "Total presets matched: $($matches.Count)"
$results = @()
foreach ($m in $matches) {
    $name = $m.Groups[1].Value
    $cat = $m.Groups[2].Value
    $json = $m.Groups[3].Value
    $nodesRegex = [regex]::new('"type":')
    $nodeCount = $nodesRegex.Matches($json).Count
    $results += [PSCustomObject]@{
        Name = $name
        Category = $cat
        Nodes = $nodeCount
    }
}
$results | Sort-Object Nodes | Format-Table -AutoSize
