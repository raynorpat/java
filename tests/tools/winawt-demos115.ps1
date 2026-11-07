param(
    [ValidateSet('Inventory','Run','Review')][string]$Mode='Inventory',
    [string]$Root=(Split-Path -Parent (Split-Path -Parent $PSScriptRoot)),
    [string]$Out,[string]$Runner,[string]$Bin,[string]$JdkHome,
    [ValidateSet('Both','Sun','Ours')][string]$Origin='Both',
    [string]$Select='.*',[int]$SettleMilliseconds=2500
)
$ErrorActionPreference='Stop'
$Root=[IO.Path]::GetFullPath($Root)
if(!$Out){$Out=Join-Path $Root 'out/winawt-demos115'}
$Out=[IO.Path]::GetFullPath($Out)
$vendorRoot=[IO.Path]::GetFullPath((Join-Path $Root 'vendor'))
if($Out -eq $vendorRoot -or $Out.StartsWith($vendorRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){
    throw 'Demo output must be outside the immutable vendor tree'
}
$demo=Join-Path $Root 'vendor/jdk1.1.5/demo'
$jdk=Join-Path $Root 'vendor/jdk1.1.5'
New-Item -ItemType Directory -Force -Path $Out | Out-Null
function Save-Json($Value,$Path){$Value | ConvertTo-Json -Depth 18 | Set-Content -Encoding UTF8 -LiteralPath $Path}
function Relative($Path){$Path.Substring($demo.Length+1).Replace('\','/')}
function Attributes([string]$Text) {
    $result=[ordered]@{}
    $pattern='(?is)([\w-]+)\s*=\s*(?:"([^"]*)"|''([^'']*)''|([^\s>]+))'
    foreach($attribute in [regex]::Matches($Text,$pattern)) {
        $value=if($attribute.Groups[2].Success){$attribute.Groups[2].Value}elseif($attribute.Groups[3].Success){$attribute.Groups[3].Value}else{$attribute.Groups[4].Value}
        $result[$attribute.Groups[1].Value.ToLowerInvariant()]=[Net.WebUtility]::HtmlDecode($value)
    }
    return $result
}
function Input-Plan([string]$Path,[string]$Kind,[string]$Text) {
    $profile='resize';$evidence='Static rendering: resize client area and verify a responsive, nonblank repaint.'
    switch -Regex ($Path) {
        '^Animator/' {$profile=if($Text -match '(?is)name\s*=\s*["'']?href'){ 'move' }else{'pause'};$evidence='Animator mousePressed pauses/resumes; observe motion before click and stable late frames after pause. Href variants receive hover only.';break}
        '^ArcTest/' {$profile='button:Fill';$evidence='ArcControls action/actionPerformed handles Fill by filling the arc.';break}
        '^CardTest/' {$profile='button:next';$evidence='CardTest action/actionPerformed selects the next CardLayout card.';break}
        '^DitherTest/' {$profile='dither';$evidence='Top Horizontal Choice Red->Green (index2), then New Image; expect green/blue instead of red/blue gradient.';break}
        '^DrawTest/' {$profile='drag';$evidence='DrawPanel mouseDown/mouseDrag or mousePressed/mouseDragged draws a line.';break}
        '^GraphicsTest/' {$profile='button:next';$evidence='GraphicsTest.action handles next by advancing the graphics card.';break}
        '^(GraphLayout|MoleculeViewer|WireFrame)/' {$profile='drag';$evidence='Graph/XYZApp/ThreeD mouseDown and mouseDrag move graph nodes or rotate the model.';break}
        '^ImageMap/' {$profile='imagemap';$evidence='ImageMap hover at280,190 enters the declared HighlightArea260,180,120,60; no click navigates.';break}
        '^JumpingBox/' {$profile='move';$evidence='MouseTrack handles mouse motion to move/track the jumping box.';break}
        '^NervousText/' {$profile='pause';$evidence='NervousText mouseDown/mousePressed pauses the text thread; observe motion before click and stable late frames after pause.';break}
        '^SortDemo/' {$profile='click';$evidence='SortItem mouseDown/mouseReleased starts the selected sorting algorithm.';break}
        '^SpreadSheet/' {$profile='sheet';$evidence='SpreadSheet.mouseDown selects a cell and keyDown edits its input area.';break}
        '^TicTacToe/' {$profile='tic';$evidence='TicTacToe mouseDown/mouseReleased chooses a board square.';break}
        '^awt-1.1/eventmodel/' {$profile='move';$evidence='Click0..3 MouseMotionListener moves the puck; later variants redispatch child events.';break}
        '^awt-1.1/symboltest/' {$profile='symbol';$evidence='SymbolTest baseText ActionListener switches the displayed Unicode base to2200.';break}
        '^awt-1.1/lightweight/.*/alternative/' {$profile='resize';$evidence='PictureFrame main hardcodes Beeper.gif, absent from every shipped variant. Verify captions, border and blank image area during resize; no rendered-photo claim.';break}
        '^awt-1.1/lightweight/(Spinner|Gauge)/' {$profile='resize';$evidence='Spinner and Gauge are animated paint demos with no input listener; resize exercises lightweight/layout repaint.';break}
        '^awt-1.1/lightweight/RoundButtons/' {$profile='round';$evidence='RoundButton mouse release at the first FlowLayout button center fires ExampleActionListener, which prints Button Pressed.';break}
        '^awt-1.1/lightweight/OpenlookButtons/' {$profile='openlook';$evidence='OpenlookButton mouse release at the first FlowLayout button center fires ExampleActionListener, which prints Button Pressed.';break}
        '^i18n/' {$profile=if($Kind -eq 'applet'){'i18n-applet'}else{'choice'};$evidence='DemoApplet Demo button creates the real frame; demo Choice item listeners change locale/format/boundary.';break}
    }
    [pscustomobject]@{profile=$profile;evidence=$evidence}
}
$html=@();$mains=@();$cases=@()
# Quote-aware tags: alt text contains literal <APPLET>, and JumpingBox has a
# commented legacy tag. Neither is an executable applet.
$tagPattern='(?is)<(?<end>/)?(?<name>applet|param)\b(?<attrs>(?:[^"''>]|"[^"]*"|''[^'']*'')*)>'
foreach($file in Get-ChildItem -LiteralPath $demo -Recurse -File -Filter '*.html' | Sort-Object FullName) {
    $text=[IO.File]::ReadAllText($file.FullName)
    $active=[regex]::Replace($text,'(?s)<!--.*?-->','')
    $applets=@();$current=$null
    foreach($tag in [regex]::Matches($active,$tagPattern)) {
        if($tag.Groups['name'].Value -ieq 'applet') {
            if($tag.Groups['end'].Success){$current=$null;continue}
            $current=[pscustomobject]@{attributes=(Attributes $tag.Groups['attrs'].Value);parameters=[ordered]@{}}
            $applets+=@($current)
        } elseif($current -and !$tag.Groups['end'].Success) {
            $attributes=Attributes $tag.Groups['attrs'].Value
            if($attributes.Contains('name')){$current.parameters[$attributes['name']]=$attributes['value']}
        }
    }
    $path=Relative $file.FullName
    $entry=[pscustomobject]@{path=$path;sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash;applets=@($applets);exclusion=if(!$applets.Count){'Documentation/index page with no active applet tag.'}else{$null}}
    $html+=@($entry)
    if($applets.Count){$cases+=@([pscustomobject]@{id=('html-'+$path.Replace('/','_').Replace('.html',''));kind='applet';path=$path;main=$null;classRoot=$null;expectedWindows=$applets.Count;cleanup='require-close';parserLimit=($path -eq 'JumpingBox/example1.html' -and $entry.sha256 -eq '00071DAD2CE0B0405DDFBF0DF4F8A89A4D942A204EFCE5EA2F2A6852C512CCE2');input=(Input-Plan $path 'applet' $active)})}
}
foreach($file in Get-ChildItem -LiteralPath $demo -Recurse -File -Filter '*.java' | Sort-Object FullName) {
    $text=[IO.File]::ReadAllText($file.FullName)
    $active=[regex]::Replace($text,'(?s:/\*.*?\*/)|//[^\r\n]*','')
    if($active -notmatch '\bpublic\s+static\s+void\s+main\s*\('){continue}
    $package=[regex]::Match($active,'(?m)^\s*package\s+([\w.]+)\s*;').Groups[1].Value
    $main=if($package){$package+'.'+$file.BaseName}else{$file.BaseName}
    $classRoot=$file.DirectoryName
    if($package){foreach($part in $package.Split('.')){$classRoot=Split-Path -Parent $classRoot}}
    $path=Relative $file.FullName
    $classFile=Join-Path $classRoot ($main.Replace('.','/')+'.class')
    $entry=[pscustomobject]@{path=$path;main=$main;classRoot=(Relative $classRoot);classPresent=(Test-Path -LiteralPath $classFile);sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash}
    $mains+=@($entry)
    $cleanup=if($path -match '^(ArcTest|CardTest|DitherTest|DrawTest)/(1\.0\.2|1\.1)/\1\.java$'){'plain-frame-bounded-termination'}else{'require-close'}
    $cases+=@([pscustomobject]@{id=('main-'+$path.Replace('/','_').Replace('.java',''));kind='main';path=$path;main=$main;classRoot=$entry.classRoot;expectedWindows=1;cleanup=$cleanup;parserLimit=$false;input=(Input-Plan $path 'main' $active)})
}
$manifest=[pscustomobject]@{schema=1;demoRoot=$demo;html=@($html);mains=@($mains);cases=@($cases);counts=[pscustomobject]@{html=$html.Count;appletPages=@($html|Where-Object {$_.applets.Count}).Count;applets=@($html|ForEach-Object {$_.applets}).Count;mains=$mains.Count;launches=$cases.Count}}
Save-Json $manifest (Join-Path $Out 'manifest.json')
$cases | Select-Object id,kind,path,main,expectedWindows,@{n='input';e={$_.input.profile}},@{n='sourceEvidence';e={$_.input.evidence}} | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $Out 'manifest.csv')
Write-Output ('Inventory: '+($manifest.counts|ConvertTo-Json -Compress))
if($Mode -eq 'Inventory'){exit 0}
if(!$Runner -or !(Test-Path -LiteralPath $Runner)){throw 'Run mode requires built awtdemos115 runner'}
if($Origin -ne 'Sun' -and (!$Bin -or !$JdkHome)){throw 'Ours requires Bin and JdkHome matching the selected build/image'}
function Quote([string]$Value){'"'+[regex]::Replace([regex]::Replace($Value,'(\\*)"','$1$1\"'),'(\\+)$','$1$1')+'"'}
$results=@();$origins=if($Origin -eq 'Both'){@('Sun','Ours')}else{@($Origin)}
$selected=@($cases | Where-Object {$_.id -match $Select})
if(!$selected.Count){throw 'Select matched no launch cases'}
$helperDirectory=Join-Path $Out 'helper'
if($Mode -eq 'Run') {
New-Item -ItemType Directory -Force -Path $helperDirectory | Out-Null
$compiler=if($Bin -and (Test-Path -LiteralPath (Join-Path $Bin 'javac.exe'))){Join-Path $Bin 'javac.exe'}else{Join-Path $jdk 'bin/javac.exe'}
$previousCompilerJavaHome=$env:JAVA_HOME
$env:JAVA_HOME=if($Bin -and $JdkHome){$JdkHome}else{$jdk}
Push-Location $PSScriptRoot
try {& $compiler -d $helperDirectory WinAwtDemoApplet115.java 2>&1 | Set-Content -LiteralPath (Join-Path $Out 'helper-compile.log');if($LASTEXITCODE){throw 'applet harness compile failed'}}
finally {Pop-Location;$env:JAVA_HOME=$previousCompilerJavaHome}
}
foreach($source in $origins) {
    $sourceBin=if($source -eq 'Sun'){Join-Path $jdk 'bin'}else{[IO.Path]::GetFullPath($Bin)}
    $sourceHome=if($source -eq 'Sun'){$jdk}else{[IO.Path]::GetFullPath($JdkHome)}
    $runtimeFiles=@()
    foreach($name in @('java.exe','javai.dll','winawt.dll','mmedia.dll','jpeg.dll','net.dll','zip.dll','math.dll','sysresource.dll')) {
        $path=Join-Path $sourceBin $name
        if(Test-Path -LiteralPath $path){$runtimeFiles+=@([pscustomobject]@{path=$path;sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash})}
    }
    $sourceDirectory=Join-Path $Out $source;New-Item -ItemType Directory -Force -Path $sourceDirectory | Out-Null
    if($Mode -eq 'Run') {
        $identity=([pscustomobject]@{origin=$source;bin=$sourceBin;jdkHome=$sourceHome;runner=$Runner;runnerSHA256=(Get-FileHash -LiteralPath $Runner -Algorithm SHA256).Hash;binaries=$runtimeFiles})
        $runtimeRecord='runtime-'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfff')+'.json'
        Save-Json $identity (Join-Path $sourceDirectory $runtimeRecord)
        if(!(Test-Path -LiteralPath (Join-Path $sourceDirectory 'runtime.json'))){Save-Json $identity (Join-Path $sourceDirectory 'runtime.json')}
    }
    $previousPath=$env:PATH;$previousJavaHome=$env:JAVA_HOME
    $env:PATH=$sourceBin+';'+$previousPath;$env:JAVA_HOME=$sourceHome
    try {foreach($case in $selected) {
        $directory=Join-Path (Join-Path $Out $source) $case.id
        if($Mode -eq 'Run') {
        New-Item -ItemType Directory -Force -Path $directory | Out-Null
        Get-ChildItem -LiteralPath $directory -File | Where-Object {$_.Name -match '^(?:(?:before|after|motion1|motion2|late)-\d\d(?:-content)?\.bmp|result\.json|launch\.json|process\.log|arguments\.txt)$'} | ForEach-Object {
            Remove-Item -LiteralPath $_.FullName
        }
        if($case.kind -eq 'applet') {
            $exe=Join-Path $sourceBin 'java.exe'
            $fileURL=([Uri](Join-Path $demo $case.path)).AbsoluteUri
            $arguments=@(('-Djava.home='+$sourceHome),'-classpath',($helperDirectory+';'+(Join-Path $jdk 'lib/classes.zip')),'WinAwtDemoApplet115',$fileURL)
            if($case.parserLimit){$arguments+=@('observe-live-parser')}
            $work=Split-Path -Parent (Join-Path $demo $case.path)
        } else {
            $exe=Join-Path $sourceBin 'java.exe';$work=Join-Path $demo $case.classRoot
            $arguments=@(('-Djava.home='+$sourceHome),'-classpath',($work+';'+(Join-Path $jdk 'lib/classes.zip')),$case.main)
        }
        $argumentFile=Join-Path $directory 'arguments.txt'
        [IO.File]::WriteAllText($argumentFile,(($arguments|ForEach-Object {Quote $_}) -join ' '),[Text.Encoding]::Default)
        & $Runner --run $exe $argumentFile $work $directory $case.input.profile $case.expectedWindows $SettleMilliseconds
        $nativeExit=$LASTEXITCODE
        $runnerHash=(Get-FileHash -LiteralPath $Runner -Algorithm SHA256).Hash
        $executableHash=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
        } else {
            $previous=Get-Content -LiteralPath (Join-Path $directory 'launch.json') -Raw | ConvertFrom-Json
            $exe=$previous.command[0];$arguments=@($previous.command|Select-Object -Skip 1);$work=$previous.workingDirectory
            $nativeExit=$previous.nativeExit;$runnerHash=$previous.runnerSHA256
            $executableHash=$previous.executableSHA256;$runtimeRecord=$previous.runtimeRecord
            if(!$runtimeRecord){$runtimeRecord='runtime.json'}
            if(!$runnerHash){$runnerHash=(Get-Content -LiteralPath (Join-Path $sourceDirectory 'runtime.json') -Raw|ConvertFrom-Json).runnerSHA256}
        }
        $resultPath=Join-Path $directory 'result.json'
        $native=if(Test-Path -LiteralPath $resultPath){Get-Content -Raw -LiteralPath $resultPath|ConvertFrom-Json}else{[pscustomobject]@{status='runner-failed';error='result.json absent'}}
        $logPath=Join-Path $directory 'process.log';$log=if(Test-Path -LiteralPath $logPath){[IO.File]::ReadAllText($logPath)}else{''}
        $errors=@([regex]::Matches($log,'(?im)^.*(?:Exception|(?:java|sun)\.[\w.]*Error|UnsatisfiedLinkError|Can.t find class|applet.*(?:failed|not initialized)|fatal error).*$')|ForEach-Object {$_.Value})
        $parseCompleted=$log -match ('(?m)^APPLET_COUNT '+$case.expectedWindows+'\s*$')
        $knownParserPending=$case.parserLimit -and !$parseCompleted -and $log -match '(?m)^APPLET_LIVE_COUNT 1\s*$' -and $log -match '(?m)^PARSER_PENDING true\s*$' -and
            $native.status -eq 'captured' -and $native.before.Count -eq 1 -and $native.after.Count -eq 1 -and
            $native.before[0].title -eq 'Applet Viewer: MouseTrack.class' -and $native.after[0].title -eq 'Applet Viewer: MouseTrack.class' -and $native.observedChange
        if($case.kind -eq 'applet' -and !$parseCompleted -and !$knownParserPending){$errors+=@('Shipped parser did not report the expected applet count.')}
        $inputErrors=@()
        if($case.input.profile -in @('round','openlook') -and $log -notmatch 'Button Pressed:'){$inputErrors+=@('Lightweight ExampleActionListener did not report Button Pressed.')}
        $boundedCleanup=$case.cleanup -eq 'plain-frame-bounded-termination' -and $native.status -eq 'cleanup-timeout' -and $native.forcedTermination -and
            $native.before.Count -eq 1 -and $native.after.Count -eq 1 -and $native.before[0].responsive -and $native.after[0].responsive -and $native.inputMessages -gt 0
        $result=[pscustomobject]@{origin=$source;id=$case.id;command=@($exe)+$arguments;workingDirectory=$work;executableSHA256=$executableHash;runnerSHA256=$runnerHash;runtimeRecord=$runtimeRecord;nativeExit=$nativeExit;native=$native;startupErrors=$errors;inputErrors=$inputErrors;cleanupPolicy=$case.cleanup;expectedBoundedCleanup=$boundedCleanup;knownShippedParserPending=$knownParserPending;accepted=(($nativeExit -eq 0 -or $boundedCleanup) -and !$errors.Count -and !$inputErrors.Count);visualParity='unreviewed';input=$case.input;artifacts=$directory}
        $results+=@($result);Save-Json $result (Join-Path $directory 'launch.json')
        Save-Json @($results) (Join-Path $Out 'results.json')
        Write-Output ($source+' '+$case.id+': '+$native.status+'; errors='+$errors.Count+'; inputErrors='+$inputErrors.Count)
    }} finally {$env:PATH=$previousPath;$env:JAVA_HOME=$previousJavaHome}
}
$comparisons=@()
if($Origin -eq 'Both') {foreach($case in $selected) {
    $sun=Join-Path (Join-Path $Out 'Sun') $case.id;$ours=Join-Path (Join-Path $Out 'Ours') $case.id
    foreach($image in Get-ChildItem -LiteralPath $sun -Filter '*.bmp' -ErrorAction SilentlyContinue) {
        $other=Join-Path $ours $image.Name
        if(Test-Path -LiteralPath $other) {
            $comparison=& $Runner --compare $image.FullName $other
            if($LASTEXITCODE -eq 0){$comparisons+=@([pscustomobject]@{id=$case.id;image=$image.Name;pixels=($comparison|ConvertFrom-Json);visualParity='unreviewed'})}
        } else {$comparisons+=@([pscustomobject]@{id=$case.id;image=$image.Name;missing='Ours';visualParity='unreviewed'})}
    }
}}
Save-Json @($comparisons) (Join-Path $Out 'comparisons.json')
$results | Select-Object origin,id,accepted,@{n='status';e={$_.native.status}},@{n='errors';e={$_.startupErrors.Count}},@{n='inputErrors';e={$_.inputErrors.Count}},artifacts | Export-Csv -NoTypeInformation -Encoding UTF8 -LiteralPath (Join-Path $Out 'results.csv')
function Html([string]$Text){[Net.WebUtility]::HtmlEncode($Text)}
$gallery=New-Object Text.StringBuilder
[void]$gallery.Append('<!doctype html><meta charset="utf-8"><title>JDK115 unchanged demo captures</title><style>body{font:14px sans-serif}table{border-collapse:collapse;width:100%;table-layout:fixed}td,th{border:1px solid #aaa;padding:5px;vertical-align:top}img{width:100%}section{margin:35px 0}.failed{color:#b00}small{display:block;overflow-wrap:anywhere}</style><h1>Unchanged demo captures</h1><p>Launch/render/input evidence only. All visual parity remains unreviewed. Click an image for its full client capture; content crops and pixel metrics are in the case directory and comparisons.json.</p>')
foreach($case in $selected) {
    [void]$gallery.Append('<section><h2>'+ (Html $case.id)+'</h2><p>'+ (Html $case.input.evidence)+'</p><table><tr>')
    foreach($source in @('Sun','Ours')){foreach($stage in @('before','after')){[void]$gallery.Append('<th>'+ $source+' '+$stage+'</th>')}}
    [void]$gallery.Append('</tr><tr>')
    foreach($source in @('Sun','Ours')){foreach($stage in @('before','after')){
        $caseResults=@($results|Where-Object {$_.origin -eq $source -and $_.id -eq $case.id})
        $caseDirectory=Join-Path (Join-Path $Out $source) $case.id
        [void]$gallery.Append('<td>')
        if($caseResults.Count){[void]$gallery.Append('<p class="'+$(if($caseResults[0].accepted){'accepted'}else{'failed'})+'">'+(Html $caseResults[0].native.status)+'</p>')}
        foreach($image in Get-ChildItem -LiteralPath $caseDirectory -Filter ($stage+'-??.bmp') -ErrorAction SilentlyContinue | Sort-Object Name) {
            $url=$source+'/'+$case.id+'/'+$image.Name
            [void]$gallery.Append('<a href="'+(Html $url)+'"><img loading="lazy" src="'+(Html $url)+'" alt="'+(Html ($case.id+' '+$image.Name))+'"></a><small>'+(Html $image.Name)+'</small>')
        }
        [void]$gallery.Append('</td>')
    }}
    [void]$gallery.Append('</tr></table></section>')
}
[IO.File]::WriteAllText((Join-Path $Out 'index.html'),$gallery.ToString(),[Text.Encoding]::UTF8)
$failures=@($results|Where-Object {!$_.accepted})
Write-Output ('Launch gates: '+($results.Count-$failures.Count)+'/'+$results.Count+' accepted; captures require visual review.')
if($failures.Count){exit 1}
