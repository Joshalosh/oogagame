
local sprite = app.activeSprite
if not sprite then return print('No active sprite') end

local local_path,title = sprite.filename:match("^(.+[/\\])(.-).([^.]*)$")

-- NOTE: Change this to whatever the local path for the game is
local export_path = "W:/oogagame/res/sprites"

local_path = export_path

-- local spriteName = app.fs.fileTitle(app.activeSprite.filename)

function layer_export()
	local fn = local_path .. "/" .. app.activeLayer.name
	app.command.ExportSpriteSheet{
		ui=false,
		type=SpriteSheetType.HORIZONTAL,
		textureFilename=fn .. '.png',
		dataFormat=SpriteSheetDataFormat.JSON_ARRAY,
		layer=app.activeLayer.name,
		trim=true,
	}
end

layer_export()