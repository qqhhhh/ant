package.path = table.concat({
	"engine/?.lua",
	"engine/?/?.lua",
	"?.lua",
}, ";")

require "bootstrap"
import_package "ant.imguibase".runtime.start("ant.test.features", 1280, 720)
