param([string]$Root=(Split-Path -Parent (Split-Path -Parent $PSScriptRoot)),[string]$Out,[string]$Runner)
$ErrorActionPreference='Stop'
if(!$Out){$Out=Join-Path $Root 'out/demo-inventory115-test'}
$script=Join-Path $PSScriptRoot 'winawt-demos115.ps1'
if(!(Test-Path -LiteralPath $script)){throw 'demo inventory implementation missing'}
& $script -Mode Inventory -Root $Root -Out $Out
$manifest=Get-Content -Raw -LiteralPath (Join-Path $Out 'manifest.json') | ConvertFrom-Json
if($manifest.html.Count -ne 64){throw 'must inventory all64 shipped HTML files'}
if(@($manifest.html | Where-Object {$_.applets.Count -gt 0}).Count -ne 58){throw 'active applet page inventory'}
if(@($manifest.html | ForEach-Object {$_.applets}).Count -ne 68){throw 'quoted alt strings or commented applets counted incorrectly'}
if($manifest.mains.Count -ne 23 -or $manifest.cases.Count -ne 81){throw 'all standalone main variants must be included'}
$jump=$manifest.html | Where-Object {$_.path -eq 'JumpingBox/example1.html'}
if($jump.applets.Count -ne 1 -or $jump.applets[0].attributes.code -ne 'Switcher.class'){throw 'commented legacy applet was not excluded'}
$sort=$manifest.html | Where-Object {$_.path -eq 'SortDemo/1.1/example1.html'}
if($sort.applets.Count -ne 3 -or $sort.applets[2].parameters.alg -ne 'QSort'){throw 'multi-applet parameters lost'}
$event=$manifest.html | Where-Object {$_.path -eq 'awt-1.1/eventmodel/index.html'}
if($event.applets.Count -ne 4){throw 'eventmodel HTML must launch all four variants'}
if(@($manifest.html | Where-Object {$_.applets.Count -eq 0 -and !$_.exclusion}).Count){throw 'missing nonapplet exclusion reasons'}
if(@($manifest.cases | Where-Object {!$_.input.evidence -or !$_.input.profile}).Count){throw 'input must have source-based rationale'}
if(@($manifest.cases | Where-Object {$_.cleanup -eq 'plain-frame-bounded-termination'}).Count -ne 8){throw 'bounded termination must be confined to the eight plain-Frame mains'}
$parserLimits=@($manifest.cases | Where-Object {$_.parserLimit})
if($parserLimits.Count -ne 1 -or $parserLimits[0].path -ne 'JumpingBox/example1.html'){throw 'live parser exception must be confined to the exact shipped malformed document'}
if($Runner){& $Runner --selftest (Join-Path $Out 'native');if($LASTEXITCODE){throw 'native runner rejection/ownership selftests failed'}}
Write-Output 'demo inventory and requested native selftests passed'
