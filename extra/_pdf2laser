#compdef pdf2laser

local args
args=(
	'(autofocus)'{--autofocus,-a}'[Enable autofocus]'
	'(job)'{--job=,-n+}'[Set display name of print job]'
	'(printer)'{--printer=,-p+}'[IP Address of target printer]'
	'(preset)'{--preset=,-P+}'[Select a default preset]'
	'(dpi)'{--dpi=,-d+}'[Resolution of the raster]'
 	'(mode)'{--mode=,-m+}'[Mode of the raster (default mono)]':'raster mode':'(mono grey colour)'
	'(raster-speed)'{--raster-speed=,-r+}'[Raster speed (0-100)]'
	'(raster-power)'{--raster-power=,-R+}'[Raster power (0-100)]'
	'(screen-size)'{--size=,-s+}'[Photograph screen size (default 8)]'
	'(optimize)'{--no-optimize,-O}'[Disable vector optimization]'
	'(frequency)'{--frequency=,-f+}'[Vector frequency]'
	'(vector-speed)'{--vector-speed=,-v+}'[Vector speed (0-100)]'
	'(vector-power)'{--vector-power=,-V+}'[Vector power (0-100)]'
	'(debug)'{--debug,-D}'[Enable debugging]'
	'(help)'{--help,-h}'[Output usage message and exit]'
	'--version[Output the version number and exit]'
)

_arguments -Ss $args[@]
